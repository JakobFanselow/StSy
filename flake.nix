{
  description = "C++ development environment with boost, fmt";
  
  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
    flake-utils.url = "github:numtide/flake-utils";
  };
  
  outputs = { self, nixpkgs, flake-utils }:
    flake-utils.lib.eachDefaultSystem (system:
      let
        pkgs = nixpkgs.legacyPackages.${system};        
        boostStatic = pkgs.boost.override { 
          enableShared = false; 
          enableStatic = true; 
        };
        fmtStatic = pkgs.fmt.override { 
          enableShared = false; 
        };
      in
      {
        devShells.default = pkgs.mkShell {
          buildInputs = with pkgs; [
            python3
            boostStatic
            fmtStatic
            #catch2
            #cloc
          ];
          
          nativeBuildInputs = with pkgs; [
            gnumake
            cmake
            libgcc
            pkg-config
            binutils
          ];
        };
      }
    );
}