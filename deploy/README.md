# Deploy Assets

`deploy/` contains checked-in packaging inputs and helper scripts.

- `shared/`: common staging and CPack configuration
- `linux/`: desktop entry, icons, and Linux package wrappers
- `macos/`: bundle metadata, icons, and macOS package wrappers
- `windows/`: Windows resources and NSIS packaging wrappers

Generated installers and staging trees belong under `dist/`, not `deploy/`.
