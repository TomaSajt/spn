with (import <nixpkgs> { });
mkShell {
  packages = [
    cmake
    eigen
    libmysqlconnectorcpp
    openssl
    zlib
    (python3.withPackages (ps: [
      ps.django
      ps.django-widget-tweaks
      ps.gunicorn
      ps.mysqlclient
      ps.pillow
      ps.pytz
      ps.sqlparse
    ]))
    tmux
  ];
  env.NIX_CFLAGS_COMPILE = "-I${lib.getDev libmysqlconnectorcpp}/include/jdbc";
}
