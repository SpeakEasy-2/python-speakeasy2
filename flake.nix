{
  description = "SpeakEasy2 python package";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-23.11";
    poetry2nix = {
      url = "github:nix-community/poetry2nix";
      inputs.nixpkgs.follows = "nixpkgs";
    };
  };

  outputs = { self, nixpkgs, poetry2nix }:
    let
      system = "x86_64-linux";
      pkgs = import nixpkgs {
        inherit system;
        overlays = [ poetry2nix.overlays.default ];
      };

      pythonEnv = pkgs.poetry2nix.mkPoetryEnv {
        projectDir = ./.;
        editablePackageSources = { speakeasy2 = ./speakeasy2; };
        preferWheels = true;
      };
    in {
      devShells.${system}.default = pkgs.mkShell {
        packages = [ pythonEnv ] ++ (with pkgs; [
          astyle
          cmake
          ninja
          gdb
          # igraph dependencies
          bison
          flex
          libxml2
          # Poetry dependencies
          poetry
        ]);
        shellHook = ''
          export OMP_NUM_THREADS=16
        '';
      };
    };
}
