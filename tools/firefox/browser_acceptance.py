#!/usr/bin/env python3
"""Test an already-running private Firefox through a loopback Marionette port.

Uses the transport from the pinned Firefox source, with no pip dependencies.
Launch Firefox with --marionette in a dedicated profile and forward its
loopback port over SSH. This script neither deploys nor launches the browser.
"""
import argparse
import base64
import importlib.util
import json
from pathlib import Path
import time


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source", type=Path, required=True)
    parser.add_argument("--port", type=int, required=True)
    parser.add_argument("--profile", required=True, help="Expected dedicated profile path on Haiku")
    parser.add_argument("--url", required=True, help="Native loopback browser-probe-server URL")
    parser.add_argument("--output", type=Path, required=True, help="New directory for evidence")
    parser.add_argument("--https-url", default="https://example.com/")
    parser.add_argument("--https-title", default="Example Domain")
    parser.add_argument("--quit", action="store_true", help="Request clean browser shutdown after success")
    parser.add_argument("--test-download", action="store_true",
                        help="Also test Downloads.fetch; launch the private browser with --remote-allow-system-access")
    args = parser.parse_args()
    if not args.url.startswith("http://127.0.0.1:"):
        parser.error("--url must point to the private loopback fixture server")
    if not args.https_url.startswith("https://"):
        parser.error("--https-url must use HTTPS")
    if args.output.exists():
        parser.error("Refusing to overwrite existing acceptance evidence")
    transport_file = args.source / "testing/marionette/client/marionette_driver/transport.py"
    spec = importlib.util.spec_from_file_location("firefox_probe_transport", transport_file)
    transport = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(transport)
    client = transport.TcpTransport("127.0.0.1", args.port, socket_timeout=150)
    args.output.mkdir(parents=True)
    evidence = {"checks": {}, "fixture_url": args.url, "https_url": args.https_url}

    def command(name, parameters=None):
        response = client.request(name, parameters or {})
        if response.error:
            raise RuntimeError(f"{name}: {json.dumps(response.error)}")
        return response.result

    def value(name, parameters=None):
        return command(name, parameters)["value"]

    def script(body):
        return value("WebDriver:ExecuteScript", {"script": body, "args": [],
            "newSandbox": True, "sandbox": "default", "line": 1,
            "filename": "haiku-browser-acceptance"})

    def screenshot(filename):
        data = value("WebDriver:TakeScreenshot", {"full": True})
        (args.output / filename).write_bytes(base64.b64decode(data, validate=True))

    try:
        client.connect()
        session = command("WebDriver:NewSession", {"acceptInsecureCerts": False})
        capabilities = session["capabilities"]
        evidence["capabilities"] = capabilities
        if capabilities.get("moz:profile") != args.profile:
            raise RuntimeError("Connected browser does not have the expected private profile")
        if capabilities.get("acceptInsecureCerts") is not False:
            raise RuntimeError("Certificate validation must remain enabled")
        command("WebDriver:SetTimeouts", {"pageLoad": 120000, "script": 30000})
        original = value("WebDriver:GetWindowHandle")
        command("WebDriver:Navigate", {"url": args.url})
        deadline = time.monotonic() + 40
        while True:
            title = value("WebDriver:GetTitle")
            if title.startswith("FIREFOX_PAGE_PROBE_PASS"):
                break
            if "FAIL" in title or time.monotonic() >= deadline:
                screenshot("fixture-failure.png")
                raise RuntimeError(f"Fixture did not pass: {title}")
            time.sleep(1)
        evidence["checks"]["page_fixture"] = title
        evidence["navigator"] = script("return {platform: navigator.platform, userAgent: navigator.userAgent};")
        evidence["features"] = script("""
            return {
              webAssembly: typeof WebAssembly !== 'undefined' &&
                WebAssembly.validate(new Uint8Array([0,97,115,109,1,0,0,0])),
              webRTC: typeof RTCPeerConnection !== 'undefined',
              webGL: !!document.createElement('canvas').getContext('webgl')
            };
        """)
        element = value("WebDriver:FindElement", {"using": "css selector", "value": "input"})
        element_id = element["element-6066-11e4-a52e-4f735466cecf"]
        command("WebDriver:ElementSendKeys", {"id": element_id, "text": "Haiku RISC-V 42"})
        if script("return document.querySelector('input').value;") != "Haiku RISC-V 42":
            raise RuntimeError("WebDriver input did not reach the page")
        evidence["checks"]["webdriver_input"] = "PASS (synthetic input; native keyboard/clipboard separate)"
        screenshot("fixture.png")
        tab = command("WebDriver:NewWindow", {"type": "tab", "focus": True})
        command("WebDriver:SwitchToWindow", {"handle": tab["handle"]})
        command("WebDriver:Navigate", {"url": args.https_url})
        https_title = value("WebDriver:GetTitle")
        current_url = value("WebDriver:GetCurrentURL")
        page_url = script("return document.documentURI;")
        if (not current_url.startswith("https://") or not page_url.startswith("https://")
                or https_title != args.https_title):
            raise RuntimeError(f"HTTPS failed or produced an error page: {page_url}")
        evidence["checks"]["https_certificate_validation"] = {"title": https_title, "url": page_url}
        screenshot("https.png")
        command("WebDriver:CloseWindow")
        command("WebDriver:SwitchToWindow", {"handle": original})
        if not value("WebDriver:GetTitle").startswith("FIREFOX_PAGE_PROBE_PASS"):
            raise RuntimeError("Original tab did not survive the HTTPS tab")
        evidence["checks"]["multiple_tabs"] = "PASS"
        if args.test_download:
            command("Marionette:SetContext", {"value": "chrome"})
            download = value("WebDriver:ExecuteAsyncScript", {
                "script": """
                    const done = arguments[arguments.length - 1];
                    const url = arguments[0];
                    (async () => {
                        const {Downloads} = ChromeUtils.importESModule('resource://gre/modules/Downloads.sys.mjs');
                        const target = PathUtils.join(PathUtils.profileDir, 'haiku-browser-download-probe.html');
                        if (await IOUtils.exists(target)) throw new Error('Download probe file already exists');
                        await Downloads.fetch(url, target);
                        const text = await IOUtils.readUTF8(target);
                        const title = new DOMParser().parseFromString(text, 'text/html').title;
                        await IOUtils.remove(target);
                        done({bytes: text.length, title});
                    })().catch(error => done({error: String(error)}));
                """, "args": [args.https_url], "newSandbox": True, "sandbox": "default",
                "line": 1, "filename": "haiku-browser-download-acceptance"})
            if download.get("error") or download.get("bytes", 0) < 100 or download.get("title") != args.https_title:
                raise RuntimeError(f"Native browser download failed: {download}")
            evidence["checks"]["download_service"] = download
            command("Marionette:SetContext", {"value": "content"})
        if args.quit:
            evidence["shutdown_request"] = command("Marionette:Quit", {"flags": ["eAttemptQuit"]})
        evidence["status"] = "PASS"
        print("FIREFOX_AUTOMATED_BROWSER_ACCEPTANCE_PASS")
    except Exception as error:
        evidence["status"] = "FAIL"
        evidence["error"] = str(error)
        raise
    finally:
        (args.output / "acceptance.json").write_text(json.dumps(evidence, indent=2) + "\n")
        client.close()


if __name__ == "__main__":
    main()
