# See: https://developers.google.com/idx/guides/customize-idx-env
{ pkgs, ... }: {
  channel = "stable-24.05";
  packages = [
    pkgs.nano
    pkgs.cmake
    pkgs.pkg-config
    pkgs.readline
    pkgs.libarchive
    pkgs.fakeroot
    pkgs.gnumake
    pkgs.gcc
    pkgs.binutils
    pkgs.libusb
    pkgs.gpgme
    pkgs.libmpc
    pkgs.mpfr
    pkgs.gmp
    pkgs.zstd
    pkgs.autoconf
    pkgs.automake
    pkgs.zlib.dev
    pkgs.python310
    pkgs.openssl
    pkgs.curl
    pkgs.zip
    pkgs.ffmpeg-full
  ];
  env = {};
  idx = {
    extensions = [ ];
    previews = {
      enable = true;
      previews = { };
    };
    workspace = {
      onCreate = {
        default.openFiles = [ ];
      };
      onStart = {
        file-server = "node .other/fs.js";
        file-watcher = ".other/runwatcher.sh";
      };
    };
  };
}
