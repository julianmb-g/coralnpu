# CoralNPU Verilator Core - Agent Instructions

## Agent Workflows & Mechanisms

### Podman Execution Mandate (Finding 333)
The host environment (including buildrabbit RPC) lacks necessary Bazel configurations for native execution. Therefore, ALL automated AI agents MUST execute `bazel` commands and E2E scripts (`run_e2e_*.sh`) hermetically inside Podman. 

Use the following command structure:
`podman run --userns=keep-id:uid=1000,gid=1000 --pids-limit=-1 -it --rm -v $PWD:$PWD -w $PWD -v coralnpu_bazel_cache_${USER}:/tmp/coralnpu_bazel_cache localhost/coralnpu-validation bash -c "git config --global --add safe.directory $PWD && <command>"`

If host-side subuid/subgid mapping fails (e.g., `Error: cannot re-exec process to join the existing user namespace`), fallback to default rootless mode by omitting the `--userns=keep-id:uid=1000,gid=1000` flag.

You are strictly forbidden from adding Podman-specific wrappers into the scripts or BUILD files themselves, as this breaks the native workflow for end-users.

### Strict Scope of Commit Claims (Finding 331)
Git commit messages must strictly correspond to tracked codebase modifications in the active workspace. Do not claim to have "committed", "pushed", or "added" changes to the wiki files within a VCS commit message. Modifying untracked wiki files and claiming them as git commit contents constitutes semantic camouflage and will result in commit rejection.
