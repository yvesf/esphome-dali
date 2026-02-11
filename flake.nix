{
  inputs.nixpkgs.url = "nixpkgs/nixos-25.11";
  outputs = { nixpkgs, ... }: {
    devShell.x86_64-linux = let pkgs = nixpkgs.legacyPackages.x86_64-linux; in pkgs.mkShell {
      buildInputs = with pkgs; [ nixpkgs-fmt cmake clang-tools catch2_3 gdb ];
    };
  };
}
