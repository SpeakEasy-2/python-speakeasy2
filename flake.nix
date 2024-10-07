{
  description = "SpeakEasy2 python package";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs";
    poetry2nix = {
      url = "github:nix-community/poetry2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, poetry2nix }:
    let
      system = "x86_64-linux";
      python = pkgs.python311;
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ poetry2nix.overlays.default ];
      };

      pythonEnv = pkgs.poetry2nix.mkPoetryEnv {
        inherit python;
        projectDir = ./.;
        editablePackageSources = { speakeasy2 = ./speakeasy2; };
        preferWheels = true;
      };
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pythonEnv ] ++ (with pkgs; [
          cmake
          clang-tools
          ninja
          gdb

          # Poetry dependencies
          poetry
        ]);
      };
    };
}
