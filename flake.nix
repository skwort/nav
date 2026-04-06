{
  description = "nav - Powerful cmdline navigation";

  inputs = {
    nixpkgs.url = "github:NixOS/nixpkgs/nixos-unstable";
  };

  outputs = {
    self,
    nixpkgs,
  }: let
    forAllSystems = nixpkgs.lib.genAttrs ["x86_64-linux" "aarch64-linux"];

    navPkg = pkgs:
      pkgs.stdenv.mkDerivation {
        pname = "nav";
        version = "0.1.0";
        src = self;
        makeFlags = ["all"];
        installPhase = ''
          mkdir -p $out/bin $out/share/nav/scripts
          install -m755 build/daemon $out/bin/navd
          install -m755 build/client $out/bin/nav-client
          install -m644 scripts/nav-bash.sh $out/share/nav/scripts/
          install -m644 scripts/nav-zsh.sh $out/share/nav/scripts/
        '';
      };
  in {
    packages = forAllSystems (system: {
      default = navPkg nixpkgs.legacyPackages.${system};
    });

    nixosModules.navd = {
      config,
      lib,
      pkgs,
      ...
    }: let
      nav = navPkg pkgs;
      cfg = config.services.navd;
    in {
      options.services.navd = {
        enable = lib.mkEnableOption "nav daemon";
        shellIntegration = {
          bash.enable = lib.mkEnableOption "nav shell integration for bash";
          zsh.enable = lib.mkEnableOption "nav shell integration for zsh";
        };
      };

      config = lib.mkIf cfg.enable (lib.mkMerge [
        {
          environment.systemPackages = [nav];

          systemd.user.services.navd = {
            description = "Nav Daemon";
            wantedBy = ["default.target"];
            serviceConfig = {
              ExecStart = "${nav}/bin/navd";
              Restart = "on-failure";
            };
          };
        }

        (lib.mkIf cfg.shellIntegration.bash.enable {
          programs.bash.interactiveShellInit = ''
            export NAV_CLIENT="${nav}/bin/nav-client"
            source ${nav}/share/nav/scripts/nav-bash.sh
          '';
        })

        (lib.mkIf cfg.shellIntegration.zsh.enable {
          programs.zsh.interactiveShellInit = ''
            export NAV_CLIENT="${nav}/bin/nav-client"
            source ${nav}/share/nav/scripts/nav-zsh.sh
          '';
        })
      ]);
    };
  };
}
