/* Browser contracts for the self-contained replay; no planner in JavaScript. */
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const { pathToFileURL } = require('node:url');
const { chromium } = require(process.env.CCPP_PLAYWRIGHT_MODULE || 'playwright');
const root = path.resolve(__dirname, '..');
const output = path.join(root, 'docs/validation/current');
fs.mkdirSync(output, { recursive: true });
(async () => {
  const launch = { headless: true };
  if (process.env.CCPP_BROWSER_EXECUTABLE) launch.executablePath = process.env.CCPP_BROWSER_EXECUTABLE;
  const browser = await chromium.launch(launch);
  const checks = [], errors = [], remote = [];
  try {
    const page = await browser.newPage({ viewport: { width: 1440, height: 1050 } });
    page.on('pageerror', error => errors.push(String(error)));
    page.on('request', request => { if (/^https?:/.test(request.url())) remote.push(request.url()); });
    await page.goto(pathToFileURL(path.join(root, 'docs/demo.html')).href);
    assert.deepEqual(errors, []);
    const scenes = await page.locator('#replay-data').textContent().then(JSON.parse);
    assert.equal(scenes.length, 6);
    for (const scene of scenes) {
      const file = JSON.parse(fs.readFileSync(path.join(root, 'docs/data', scene.scenario + '.json'), 'utf8'));
      assert.deepEqual(scene, file, 'embedded data must exactly match actual C++ JSON');
      await page.selectOption('#scene', scene.scenario);
      assert.equal(await page.locator('#targets').innerText(), '0 / ' + scene.confirmed_targets);
      await page.locator('#progress').evaluate(el => { el.value = el.max; el.dispatchEvent(new Event('input')); });
      assert.equal(await page.locator('#targets').innerText(), scene.confirmed_targets + ' / ' + scene.confirmed_targets);
      assert.equal(await page.locator('#chunk').innerText(), scene.chunks + ' / ' + scene.chunks);
      assert.equal(await page.locator('#state').innerText(), 'Reachable targets complete');
      checks.push('exact data, final targets/chunks and state: ' + scene.scenario);
    }
    await page.selectOption('#scene', 'obstacles');
    await page.selectOption('#speed', '4');
    await page.locator('#play').click();
    await page.waitForFunction(() => Number(document.querySelector('#progress').value) > 3, { timeout: 5000 });
    await page.locator('#play').click();
    const stopped = await page.locator('#progress').inputValue();
    await page.waitForTimeout(150);
    assert.equal(await page.locator('#progress').inputValue(), stopped);
    checks.push('4x speed, play and pause');
    await page.locator('#progress').evaluate(el => { el.value = 190; el.dispatchEvent(new Event('input')); });
    assert.equal(await page.locator('#path-count').innerText(), '190 / 368');
    await page.locator('#full-route').check();
    await page.screenshot({ path: path.join(output, 'demo_desktop.png'), fullPage: true });
    checks.push('seek, complete-route toggle, desktop screenshot');
    await page.locator('#reset').click();
    assert.equal(await page.locator('#targets').innerText(), '0 / 292');
    await page.setViewportSize({ width: 390, height: 844 });
    await page.screenshot({ path: path.join(output, 'demo_mobile.png'), fullPage: true });
    assert.equal(await page.evaluate(() => document.documentElement.scrollWidth > innerWidth), false);
    checks.push('restart and narrow-screen layout');
    await page.setViewportSize({ width: 1280, height: 455 });
    await page.goto(pathToFileURL(path.join(root, 'assets/banner.svg')).href);
    await page.screenshot({ path: path.join(output, 'banner.png') });
    assert.deepEqual(errors, []);
    assert.deepEqual(remote, []);
    checks.push('no page errors; no HTTP requests');
    const report = { status: 'passed', browser: browser.version(), playwright: require((process.env.CCPP_PLAYWRIGHT_MODULE || 'playwright') + '/package.json').version, checks };
    fs.writeFileSync(path.join(output, 'browser.json'), JSON.stringify(report, null, 2) + '\n');
    console.log(JSON.stringify(report, null, 2));
  } finally { await browser.close(); }
})().catch(error => { console.error(error); process.exitCode = 1; });
