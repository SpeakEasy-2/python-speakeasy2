{
  description = "SpeakEasy2 python package";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";

    pyproject-nix = {
      url = "github:pyproject-nix/pyproject.nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    uv2nix = {
      url = "github:pyproject-nix/uv2nix";
      inputs.pyproject-nix.follows = "pyproject-nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };

    pyproject-build-systems = {
      url = "github:pyproject-nix/build-system-pkgs";
      inputs.pyproject-nix.follows = "pyproject-nix";
      inputs.uv2nix.follows = "uv2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { nixpkgs, pyproject-nix, uv2nix, pyproject-build-systems, ... }:
    let
      system = "x86_64-linux";
      pkgs = nixpkgs.legacyPackages.${system};

      pythonBase = pkgs.callPackage pyproject-nix.build.packages {
        python = pkgs.python3;
      };

      workspace = uv2nix.lib.workspace.loadWorkspace { workspaceRoot = ./.; };
      overlay = workspace.mkPyprojectOverlay { sourcePreference = "wheel"; };
      pyProjectOverrides = final: prev: {
        scikit-build-core = prev.scikit-build-core.overrideAttrs
          (old: { propagatedBuildInputs = [ pkgs.cmake pkgs.ninja ]; });
      };
      pythonSet = pythonBase.overrideScope (nixpkgs.lib.composeManyExtensions [
        pyproject-build-systems.overlays.wheel
        pyProjectOverrides
        overlay
      ]);

      editableOverlay =
        workspace.mkEditablePyprojectOverlay { root = "$REPO_ROOT"; };

      editablePythonSet = pythonSet.overrideScope editableOverlay;
      venv = pythonSet.mkVirtualEnv "se2-dev-env" workspace.deps.all;
    in {
      devShells.${system}.default =
        (pkgs.mkShell.override { stdenv = pkgs.clangStdenv; }) {
          packages = [ venv ] ++ (with pkgs; [
            cmake
            clang-tools
            ninja
            gnumake
            bear
            gdb

            # uv dependencies
            uv
          ]);

          env = {
            UV_NO_SYNC = "1";
            UV_PYTHON = editablePythonSet.python.interpreter;
            UV_PYTHON_DOWNLOADS = "never";
          };

          shellHook = ''
            unset PYTHONPATH
            export REPO_ROOT=$(git rev-parse --show-toplevel)
          '';
        };
    };
}
