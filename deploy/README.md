# Deploy Assets

`deploy/` contains checked-in packaging inputs and helper scripts.

- `shared/`: common staging and CPack configuration
- `linux/`: desktop entry, icons, and Linux package wrappers
- `macos/`: bundle metadata, icons, and macOS archive wrappers
- `windows/`: Windows resources and archive wrappers

Use `deploy/shared/stage.sh` when you want a raw install tree. Use the archive
and package wrappers when you want CI-ready payloads from CPack.

Generated installers and staging trees belong under `dist/`, not `deploy/`.
