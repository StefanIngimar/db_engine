#!/usr/bin/env bash
set -e

gh issue create --title "LeafPage: RemoveAt + IsUnderflow" \
  --body "Add RemoveAt(index) to shift/compact entries after a delete. Add IsUnderflow() returning true when cell count < MIN_CELLS (typically MAX_CELLS/2) and the node isn't the root."

gh issue create --title "LeafPage: RedistributeFrom sibling" \
  --body "Borrow one entry from a left or right sibling when this leaf underflows but the sibling has spare entries. Must return the new separator key so the parent can be updated. Test with small MAX_CELLS to force this without thousands of inserts."

gh issue create --title "LeafPage: MergeFrom sibling" \
  --body "Absorb all of a sibling's entries into this leaf when redistribution isn't possible (sibling also at MIN_CELLS). Caller is responsible for deallocating the emptied sibling page and removing the separator key from the parent."

gh issue create --title "InternalPage: RemoveAt + IsUnderflow" \
  --body "Same as LeafPage::RemoveAt/IsUnderflow but must also shift child pointers, not just keys."

gh issue create --title "InternalPage: RedistributeFrom sibling" \
  --body "Borrow a (key, child) pair from a sibling. Needs the parent's current separator key as input and returns the updated one — internal redistribution rotates through the parent, unlike leaf redistribution."

gh issue create --title "InternalPage: MergeFrom sibling" \
  --body "Merge two internal nodes plus the separator key pulled down from the parent into one node. Reparent all moved child pages to point at the surviving node."

gh issue create --title "BPlusTree::Remove driver + FixUnderflow recursion" \
  --body "Wire up the full delete path: find leaf, remove key, then propagate underflow fixes up the tree using the saved root-to-leaf path (don't re-search from root). A merge at one level can underflow its parent — this must recurse until no underflow remains or the root is reached."

gh issue create --title "Root collapse after cascading merge" \
  --body "If merges propagate all the way up and the root (internal node) is left with zero keys, its single remaining child must become the new root. Handle both root-is-leaf and root-is-internal cases."

gh issue create --title "Unit tests: B+Tree deletion" \
  --body "Cover: delete from a leaf with no underflow, delete triggering redistribution (both directions), delete triggering merge, delete triggering a merge cascade up multiple levels, delete that collapses the root. Use a small MAX_CELLS constant to make multi-level cases reachable with ~20-30 inserts."

gh issue create --title "Value + Schema types" \
  --body "Add Value (tagged INTEGER/VARCHAR) and Schema/Column types. This replaces the fixed Row struct now that CREATE TABLE means schemas are no longer hardcoded at compile time."

gh issue create --title "Catalog: CreateTable / GetSchema / GetIndex" \
  --body "In-memory table name -> Schema registry, plus one BPlusTree index per table. Needs a plan for persisting the catalog itself across restarts (even a simple flat file is fine for now)."

gh issue create --title "Lexer: string to token stream" \
  --body "Tokenize SELECT/INSERT/DELETE/CREATE/TABLE/INTO/VALUES/WHERE/FROM keywords, identifiers, int and string literals, and punctuation (= * , ( ) ;)."

gh issue create --title "Parser: SELECT statement" \
  --body "Recursive descent parse of SELECT * FROM table [WHERE column = literal]. Produces a SelectStmt AST node."

gh issue create --title "Parser: INSERT statement" \
  --body "Parse INSERT INTO table VALUES (...). Produces an InsertStmt AST node with a Value list."

gh issue create --title "Parser: DELETE statement" \
  --body "Parse DELETE FROM table [WHERE column = literal]. Produces a DeleteStmt AST node."

gh issue create --title "Parser: CREATE TABLE statement" \
  --body "Parse CREATE TABLE name (col type, ...). Produces a CreateTableStmt AST node feeding into Catalog::CreateTable."

gh issue create --title "Executor: ExecCreateTable + ExecInsert" \
  --body "Wire CreateTableStmt to Catalog::CreateTable. Wire InsertStmt to schema-driven row serialization + BPlusTree::Insert."

gh issue create --title "Executor: ExecSelect with point-lookup vs scan branch" \
  --body "If WHERE matches the indexed key, do a direct BPlusTree point lookup. Otherwise fall back to a full leaf-chain scan via next_leaf pointers. This branch is the seed of a real query planner — document it as such in the README."

gh issue create --title "Executor: ExecDelete" \
  --body "Wire DeleteStmt to BPlusTree::Remove, using the same point-lookup-vs-scan logic as ExecSelect to find matching rows first."

gh issue create --title "Unit tests: parser" \
  --body "One test per statement type (SELECT, INSERT, DELETE, CREATE TABLE) verifying the produced AST, plus a couple of malformed-input cases that should fail cleanly."

gh issue create --title "Unit tests: executor end-to-end" \
  --body "CREATE TABLE -> INSERT rows -> SELECT with and without WHERE -> DELETE -> SELECT again to confirm removal, all driven through the parser+executor rather than calling BPlusTree directly."

echo "Done. Run 'gh issue list' to confirm."
