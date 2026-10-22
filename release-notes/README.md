# Unreleased items of the release notes

A change that users notice adds its item of `ReleaseNotes.md` as a file of this directory, not to `ReleaseNotes.md`,
so pull requests do not conflict there:

- the file is named by the ticket, `MDAPI-455.md`, or, for a change without a ticket, with a short name such as
  `graal-sdk-3.9.1.md`;
- it holds the item in the Markdown of `ReleaseNotes.md`, nested items included:

  ```markdown
  * **\[MDAPI-455]\[C++]** Short description of the change.
      * A detail.
  ```

A release or a pre-release moves the items under the heading of the version in `ReleaseNotes.md` and removes the
files (see `HOW_TO_RELEASE.md`), in this order: the items marked `**\[BREAKING]**` (at the top level), then the
files named by a ticket, the newest first, then the others by name. A trial release (`vX.Y.Z-draftN`) shows them as
the notes of its draft GitHub release.
