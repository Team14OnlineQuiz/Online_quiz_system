# Implementation baseline

The team selected the attached Deliverable 1 (version 1.0, 28 September 2026) when creating issues FR-01 to FR-11, NFR-01 to NFR-04, and SEC-01 to SEC-04. Those issue descriptions are the implementation acceptance criteria.

The archived SRS and architecture documents in `docs/legacy` describe a different, older scope: participant names, no learner history, the first ten bank questions, and no third-party libraries. Their requirement and test identifiers are not interchangeable with the selected attachment.

The selected attachment explicitly requires a vetted adaptive hashing library. This implementation uses official libsodium's Argon2id API with independent random salts and interactive cost parameters (64 MiB memory). Encoded hashes retain algorithm, salt, and cost parameters. There is no plaintext-password storage or custom hash implementation.

Reference: https://doc.libsodium.org/password_hashing/default_phf

Windows validation uses the official libsodium 1.0.22 MinGW archive, SHA-256 `1d99e0afaf27bce664249232e9dc628ae6bb7b49f0ab53ad6db372b028a35d9d`. The archive and compiler binaries are not committed.

Shared types and repository interfaces are integration foundations for other members' stories. Adding an interface or fixture does not mean their assigned issues are completed. The console remains local and single-user; there is no HTTP service, browser frontend, or network quiz play.
