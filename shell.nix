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
}
