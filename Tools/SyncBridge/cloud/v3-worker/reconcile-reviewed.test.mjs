import { test } from 'node:test';
import assert from 'node:assert/strict';
import { assertReviewablePlan, counts, parseArgs } from './reconcile-reviewed.mjs';

test('an apply requires an exact review hash and refuses force and omitted pins', () => {
  assert.throws(() => parseArgs(['--mode', 'apply', '--review', 'C:\\review.json']), /ModeArgumentMismatch/);
  assert.throws(() => parseArgs(['--mode', 'apply', '--force', 'true']), /UnexpectedOrDuplicateArgument/);
  assert.throws(() => parseArgs(['--mode', 'apply', '--review', 'C:\\review.json', '--review-sha', 'bad']), /InvalidReviewPins/);
  assert.equal(parseArgs(['--mode', 'apply', '--review', 'C:\\review.json', '--review-sha', 'a'.repeat(64)]).mode, 'apply');
});
test('a recovery never turns unknown paths, ownership changes or wrong types into an ordinary plan', () => {
  const base = { actions: [{ action: 'edit', path: 'Source/Main.cpp' }], impact: 'binary-build-required', holds: [] };
  assert.equal(assertReviewablePlan(base), base);
  assert.doesNotThrow(() => assertReviewablePlan({ ...base, holds: [{ path: 'Source/Main.cpp', reason: 'opened-overlap' }] }));
  for (const reason of ['unreviewed-path', 'windows-case-collision', 'p4-filetype-unverified-or-disallowed']) {
    assert.throws(() => assertReviewablePlan({ ...base, holds: [{ path: 'Source/Main.cpp', reason }] }), /RecoveryPlanHold/);
  }
  assert.throws(() => assertReviewablePlan({ ...base, impact: 'metadata-only' }), /RecoveryRequiresNormalBuildAndPCB/);
  assert.throws(() => assertReviewablePlan({ ...base, actions: [] }), /RecoveryActionCountInvalid/);
});
test('the action summary keeps deletions visible for explicit review', () => {
  assert.deepEqual(counts([{ action: 'add' }, { action: 'edit' }, { action: 'edit' }, { action: 'delete' }]),
    { add: 1, edit: 2, delete: 1 });
});
