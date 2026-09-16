# Verse OS VPS deployment

Deployed 2026-09-16 to https://verseos.org on the user-provided Ubuntu VPS.

- Server: `172.236.243.211`; SSH user: `root`.
- Local SSH identity: `/Users/arma/.ssh/verseos_deploy` (never commit the private key).
- Verified server ED25519 fingerprint: `SHA256:Zx6G6CV4fL1sBQgLvQnbzXWNGtMsX0SCFFXxMIpC9m4`.
- Web server: Ubuntu Caddy package, enabled at boot, automatic HTTPS.
- Active content: `/srv/verseos/current`, symlink to `/srv/verseos/releases/20260916-4e5e7ed`.
- Server configuration: `/etc/caddy/Caddyfile`; source in this directory.
- Original configuration backup: `/srv/verseos/config-backups/Caddyfile.initial`.

Only `dist/index.html`, `dist/style.css`, and `dist/web_hero.PNG` were uploaded. The existing Sites hosting metadata is historical; production DNS points to this VPS, not Sites. Only the apex domain is configured, not `www`.

## Future updates

Upload the public files into a new uniquely named release directory under `/srv/verseos/releases`. Verify checksums and permissions (directories 755, files 644), then switch `/srv/verseos/current` using a temporary symlink and an atomic rename. Keep previous releases for rollback. Static content changes do not require a Caddy restart.

For configuration changes, back up the active configuration, validate the candidate with `caddy validate --config <candidate> --adapter caddyfile`, install it, then run `systemctl reload caddy`. Do not change SSH authentication or firewall rules as part of a routine content deployment.

Verify HTTPS and asset responses after deployment. Roll back content by atomically switching `current` to the previous release.

## Initial verification

HTTP redirected to HTTPS (308), HTTPS returned 200 with certificate verification enabled, and the served HTML checksum matched the local file. All three uploaded files matched local SHA-256 checksums.
