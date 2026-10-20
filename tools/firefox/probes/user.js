// Private browser acceptance profile; these are not distro-wide defaults.
user_pref("browser.shell.checkDefaultBrowser", false);
user_pref("browser.startup.homepage_override.mstone", "ignore");
user_pref("browser.startup.page", 0);
user_pref("startup.homepage_welcome_url", "");
user_pref("browser.aboutwelcome.enabled", false);
user_pref("browser.newtabpage.enabled", false);
user_pref("datareporting.policy.dataSubmissionEnabled", false);
user_pref("toolkit.telemetry.enabled", false);
user_pref("toolkit.telemetry.unified", false);
user_pref("browser.newtabpage.activity-stream.feeds.telemetry", false);
user_pref("gfx.webrender.software", true);
user_pref("webgl.disabled", true);
// Only used when explicitly launched with --marionette for acceptance tests.
user_pref("marionette.port", 28281);
