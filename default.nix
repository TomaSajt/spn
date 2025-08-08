with (import <nixpkgs> { });
{
  gameserver = stdenv.mkDerivation {
    name = "spn-gameserver";
    src = nix-gitignore.gitignoreSource [ ] ./gameserver;

    nativeBuildInputs = [ cmake ];

    buildInputs = [
      eigen
      libmysqlconnectorcpp
    ];

    installPhase = ''
      install -Dm755 GameServer -t $out/bin/
    '';

    meta.mainProgram = "GameServer";
  };

  relayserver = stdenv.mkDerivation {
    name = "spn-relayserver";
    src = nix-gitignore.gitignoreSource [ ] ./relayserver;

    nativeBuildInputs = [ cmake ];

    buildInputs = [
      eigen
      openssl
      zlib
    ];

    installPhase = ''
      install -Dm755 relayserver/RelayServer -t $out/bin/
    '';

    meta.mainProgram = "RelayServer";
  };
}
