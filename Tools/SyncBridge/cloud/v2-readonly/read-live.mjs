// Read-only snapshot: git ls-remote/ls-tree/cat-file/diff; p4 changes/fstat/opened.
// No fetch, sync, edit, reconcile, submit, schedule, or file writes.
import { spawnSync } from 'node:child_process';
import { createHash } from 'node:crypto';
import { readFileSync } from 'node:fs';
import { dirname, isAbsolute, join, resolve } from 'node:path';
import { fileURLToPath } from 'node:url';
import { GENESIS_GIT, GENESIS_P4, UHT_PROJECT, classifyV2, detectCommitState, planTerminal, assessProvenance } from './planner.mjs';

const HERE = dirname(fileURLToPath(import.meta.url));
const UTF8 = new TextDecoder('utf-8', { fatal: true });
const GBK = new TextDecoder('gbk', { fatal: true });
const MAX_BUFFER = 256 * 1024 * 1024;

function command(binary, args, cwd, { input, allowMissing = false } = {}) {
  const result = spawnSync(binary, args, { cwd, input, encoding: null, maxBuffer: MAX_BUFFER,
    windowsHide: true, env: { ...process.env, GIT_TERMINAL_PROMPT: '0' } });
  if (result.error) throw result.error;
  if (result.status !== 0 && !allowMissing) {
    throw new Error(`${binary} read command failed (${result.status}): ${result.stderr.toString('utf8').slice(0, 500)}`);
  }
  return result;
}

function decodeP4(raw) {
  try { return UTF8.decode(raw); } catch { return GBK.decode(raw); }
}

export function parseP4Tags(raw) {
  const records = [];
  let current = {};
  for (const line of decodeP4(raw).split(/\r?\n/)) {
    if (!line) {
      if (Object.keys(current).length) records.push(current);
      current = {};
      continue;
    }
    const match = /^\.\.\. ([^ ]+) ?(.*)$/.exec(line);
    if (!match) continue;
    if (Object.hasOwn(current, match[1])) throw new Error(`Repeated P4 field ${match[1]}`);
    current[match[1]] = match[2];
  }
  if (Object.keys(current).length) records.push(current);
  return records;
}

export function p4(args, cwd, allowMissing = false) {
  const result = command('p4', ['-ztag', ...args], cwd, { allowMissing });
  const records = parseP4Tags(result.stdout);
  const errors = records.filter((x) => x.code === 'error');
  if (result.status !== 0 || errors.length) {
    const detail = `${decodeP4(result.stdout)} ${decodeP4(result.stderr)}`;
    if (allowMissing && /no such file\(s\)|file\(s\) not opened anywhere/i.test(detail)) return [];
    throw new Error(`P4 read failed: ${args[0]}: ${detail.slice(0, 500)}`);
  }
  return records;
}

export function parseGitTree(raw) {
  if (raw.length && raw[raw.length - 1] !== 0) throw new Error('Malformed NUL-delimited Git tree');
  const entries = new Map();
  for (const record of raw.toString('binary').split('\0')) {
    if (!record) continue;
    const tab = record.indexOf('\t');
    if (tab < 0) throw new Error('Malformed Git tree entry');
    const [mode, type, oid, length] = record.slice(0, tab).trim().split(/\s+/);
    const path = UTF8.decode(Buffer.from(record.slice(tab + 1), 'binary'));
    if (!/^[0-9a-f]{40,64}$/.test(oid) || !Number.isSafeInteger(Number(length)) || entries.has(path)) {
      throw new Error(`Invalid/duplicate Git tree entry ${path}`);
    }
    entries.set(path, { mode, type, oid, size: Number(length) });
  }
  return entries;
}

export function readGit(gitRepo, sha, policy) {
  const raw = command('git', ['-C', gitRepo, 'ls-tree', '-rlz', sha, '--'], gitRepo).stdout;
  const tree = parseGitTree(raw);
  const selected = [...tree].filter(([path]) => classifyV2(path).eligible);
  const violations = [];
  for (const [path, value] of selected) {
    const max = classifyV2(path).kind === 'docs' ? policy.maxDocsBytes : policy.maxTextBytes;
    if (value.type !== 'blob' || value.mode !== '100644' || value.size > max) {
      violations.push({ path, reason: 'git-object-or-size-invalid', mode: value.mode, size: value.size });
    }
  }
  if (violations.length) return { entries: new Map(), tree, violations };
  const input = Buffer.from(selected.map(([, value]) => value.oid).join('\n') + '\n', 'ascii');
  const output = selected.length ? command('git', ['-C', gitRepo, 'cat-file', '--batch'], gitRepo, { input }).stdout : Buffer.alloc(0);
  const entries = new Map();
  let offset = 0;
  for (const [path, value] of selected) {
    const newline = output.indexOf(10, offset);
    if (newline < 0) throw new Error(`Truncated Git blob header: ${path}`);
    const header = output.subarray(offset, newline).toString('ascii');
    const match = /^([0-9a-f]{40,64}) blob (\d+)$/.exec(header);
    if (!match || match[1] !== value.oid || Number(match[2]) !== value.size) throw new Error(`Git blob mismatch: ${path}; header=${JSON.stringify(header)}; expected=${value.oid}/${value.size}`);
    const end = newline + 1 + value.size;
    if (end >= output.length || output[end] !== 10) throw new Error(`Truncated Git blob: ${path}`);
    const bytes = output.subarray(newline + 1, end);
    entries.set(path, { digest: createHash('md5').update(bytes).digest('hex'), size: value.size,
      sha256: createHash('sha256').update(bytes).digest('hex'), oid: value.oid });
    offset = end + 1;
  }
  if (offset !== output.length) throw new Error('Trailing Git cat-file bytes');
  return { entries, tree, violations };
}

export function readChangedPaths(gitRepo, from, to) {
  if (from === to) return [];
  const raw = command('git', ['-C', gitRepo, 'diff', '--name-only', '-z', '--no-renames', from, to, '--'], gitRepo).stdout;
  if (raw.length && raw[raw.length - 1] !== 0) throw new Error('Malformed Git diff');
  return raw.toString('binary').split('\0').filter(Boolean).map((x) => UTF8.decode(Buffer.from(x, 'binary')));
}

function p4Patterns(depot) {
  return [`${depot}/Source/...`, `${depot}/Config/...`, `${depot}/Plugins/.../Source/...`,
    `${depot}/Plugins/.../Config/...`, `${depot}/Plugins/.../*.uplugin`, `${depot}/Content/Script/...`,
    `${depot}/DevKit.uproject`, `${depot}/Docs/...`, `${depot}/${UHT_PROJECT}`,
    `${depot}/Plugins/YogVision/Shaders/...`, `${depot}/Plugins/YogVision/Resources/...`];
}

export function p4TaggedItemToEntry(item, depot) {
  if (!item.depotFile?.startsWith(`${depot}/`)) throw new Error(`Unexpected P4 path ${item.depotFile}`);
  // P4 fstat wildcards can return a depotFile-only placeholder for a deleted/nonexistent path.
  if (!item.headRev || ['delete', 'move/delete'].includes(item.headAction)) return null;
  const path = item.depotFile.slice(depot.length + 1);
  if (!classifyV2(path).eligible) return null;
  const size = Number(item.fileSize);
  if (!/^[0-9a-f]{32}$/i.test(item.digest ?? '') || !Number.isSafeInteger(size)) {
    return { path, violation: { path, reason: 'missing-or-invalid-p4-digest' } };
  }
  return { path, value: { digest: item.digest.toLowerCase(), size, rev: Number(item.headRev), type: item.headType } };
}

export function readP4(cwd, depot) {
  const entries = new Map();
  const violations = [];
  for (const pattern of p4Patterns(depot)) {
    for (const item of p4(['fstat', '-Ol', '-T', 'depotFile,headRev,headAction,digest,fileSize,headType', pattern], cwd, true)) {
      const parsed = p4TaggedItemToEntry(item, depot);
      if (!parsed) continue;
      if (parsed.violation) { violations.push(parsed.violation); continue; }
      const { path, value } = parsed;
      const prior = entries.get(path);
      if (prior && JSON.stringify(prior) !== JSON.stringify(value)) throw new Error(`Conflicting P4 metadata ${path}`);
      entries.set(path, value);
    }
  }
  return { entries, violations };
}

export function readOpened(cwd, depot) {
  return p4(['opened', '-a', `${depot}/...`], cwd, true).filter((x) => x.depotFile?.startsWith(`${depot}/`))
    .map((x) => x.depotFile.slice(depot.length + 1));
}

export function checkManualTransition(item, transition, depot) {
  const depotFiles = Object.keys(item).filter((key) => /^depotFile\d+$/.test(key));
  return item.change === String(transition.change) && item.status === 'submitted' &&
    item.desc?.trimStart().startsWith(transition.descriptionPrefix) && depotFiles.length === 1 &&
    item.depotFile0 === `${depot}/${transition.path}` && item.action0 === transition.action &&
    item.type0 === 'binary' && Number(item.rev0) === transition.revision &&
    Number(item.fileSize0) === transition.bytes && item.digest0?.toLowerCase() === transition.md5 &&
    transition.path === UHT_PROJECT;
}

function parseArgs(args) {
  if (args.includes('--help')) return { help: true };
  const out = {};
  for (let i = 0; i < args.length; i += 2) {
    const key = args[i]?.slice(2);
    if (!args[i]?.startsWith('--') || !['git-repo', 'p4-cwd'].includes(key) || !args[i + 1] || Object.hasOwn(out, key)) {
      throw new Error('Usage: node read-live.mjs --git-repo <absolute> --p4-cwd <absolute>');
    }
    out[key] = args[i + 1];
  }
  if (!isAbsolute(out['git-repo'] ?? '') || !isAbsolute(out['p4-cwd'] ?? '')) {
    throw new Error('Both --git-repo and --p4-cwd must be absolute paths');
  }
  return { gitRepo: resolve(out['git-repo']), p4Cwd: resolve(out['p4-cwd']) };
}

function main() {
  const args = parseArgs(process.argv.slice(2));
  if (args.help) { process.stdout.write('Usage: node read-live.mjs --git-repo <absolute> --p4-cwd <absolute>\n'); return; }
  const policyBytes = readFileSync(join(HERE, 'policy.v2.json'));
  const policy = JSON.parse(UTF8.decode(policyBytes));
  if (policy.schemaVersion !== 2 || policy.genesisGitCommit !== GENESIS_GIT || policy.genesisP4Change !== GENESIS_P4 ||
      policy.depot !== '//Dev01/main' || policy.exactExceptions?.length !== 1 || policy.exactExceptions[0] !== UHT_PROJECT ||
      policy.manualP4Transitions?.length !== 2 || policy.manualP4Transitions[0]?.change !== 168 ||
      policy.manualP4Transitions[0]?.disposition !== 'rejected-byte-parity' ||
      policy.manualP4Transitions[1]?.change !== 169 ||
      policy.manualP4Transitions[1]?.disposition !== 'reviewed-byte-parity-baseline') {
    throw new Error('Unexpected v2 prototype policy');
  }
  const policySha256 = createHash('sha256').update(policyBytes).digest('hex');
  const remoteLine = command('git', ['-C', args.gitRepo, 'ls-remote', '--exit-code', 'origin', 'refs/heads/main'], args.gitRepo)
    .stdout.toString('ascii').trim();
  const remote = /^([0-9a-f]{40})\s+refs\/heads\/main$/.exec(remoteLine)?.[1];
  if (!remote) throw new Error('GitHub main SHA response is invalid');
  const cached = command('git', ['-C', args.gitRepo, 'cat-file', '-e', `${remote}^{commit}`], args.gitRepo, { allowMissing: true }).status === 0;
  const descendant = cached && command('git', ['-C', args.gitRepo, 'merge-base', '--is-ancestor', GENESIS_GIT, remote], args.gitRepo, { allowMissing: true }).status === 0;
  const detection = detectCommitState({ genesis: GENESIS_GIT, remote, cached, descendant });
  const p4Change = Number(p4(['changes', '-m1', `${policy.depot}/...`], args.p4Cwd)[0]?.change);
  if (!Number.isSafeInteger(p4Change)) throw new Error('Could not read P4 head change');
  const transitionEvidence = policy.manualP4Transitions.map((transition) => ({
    change: transition.change,
    disposition: transition.disposition,
    exactSubmittedChangeVerified: checkManualTransition(Object.assign({}, ...p4(['describe', '-s', String(transition.change)], args.p4Cwd)), transition, policy.depot),
  }));
  const output = { schemaVersion: 2, prototype: true, readOnly: true, genesisGitCommit: GENESIS_GIT,
    genesisP4Change: GENESIS_P4, manualP4Transitions: policy.manualP4Transitions,
    policySha256, remoteGitCommit: remote, detection, p4HeadChange: p4Change, transitionEvidence,
    parityP4Baseline: 169, releaseGate: 'HOLD_UNVERIFIED_CL169_PCB_BASELINE' };
  if (p4Change < GENESIS_P4 || detection.status === 'HOLD' || detection.status === 'NEEDS_FETCH') {
    output.status = 'HOLD';
    output.reason = p4Change < GENESIS_P4 ? 'p4-before-genesis' : detection.status === 'NEEDS_FETCH' ? 'remote-commit-not-cached-no-fetch' : detection.reason;
    process.stdout.write(`${JSON.stringify(output, null, 2)}\n`);
    return;
  }
  const git = readGit(args.gitRepo, remote, policy);
  const p4State = readP4(args.p4Cwd, policy.depot);
  const changedPaths = readChangedPaths(args.gitRepo, GENESIS_GIT, remote);
  const openedPaths = readOpened(args.p4Cwd, policy.depot);
  const plan = planTerminal({ gitEntries: git.entries, p4Entries: p4State.entries, changedPaths, openedPaths });
  output.gitEligibleCount = git.entries.size;
  output.p4EligibleCount = p4State.entries.size;
  output.gitChangedPathCount = changedPaths.length;
  output.skippedAssetOrGenerated = plan.skipped.filter((x) => ['asset-skip', 'generated-skip'].includes(x.reason));
  output.actions = plan.actions;
  output.impact = plan.impact;
  output.holds = [...git.violations, ...p4State.violations, ...plan.holds];
  if (transitionEvidence.some((item) => !item.exactSubmittedChangeVerified)) {
    output.holds.push({ reason: 'manual-transition-evidence-mismatch' });
  }
  output.openedUnrelatedCount = plan.openedUnrelated.length;
  output.terminalStatus = plan.status;
  output.status = output.holds.length ? 'HOLD' : plan.status;
  const provenance = assessProvenance({ detectionStatus: detection.status, actionCount: plan.actions.length,
    p4Head: p4Change, reviewedParityP4: 169 });
  output.provenanceGate = provenance.reason;
  if (provenance.status === 'HOLD') {
    output.status = 'HOLD';
    output.holds.push({ reason: provenance.reason });
  }
  process.stdout.write(`${JSON.stringify(output, null, 2)}\n`);
}

if (process.argv[1] && resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  try { main(); } catch (error) { process.stderr.write(`${error.stack ?? error}\n`); process.exitCode = 1; }
}

