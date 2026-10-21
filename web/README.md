# cirque-input-module — Web UI

React + Vite + TypeScript front-end for the DYA Studio tab of the
`cirque-input-module` ZMK firmware. Lets you connect to a keyboard
running the cirque driver over Web Serial (USB) or Web Bluetooth and
tweak the cirque trackpad settings in real time.

The page itself is a thin shell around
[`@cormoran/zmk-studio-react-hook`](https://github.com/cormoran/react-zmk-studio),
which handles the Web Serial / Web Bluetooth transports, auto-reconnect,
and the Studio unlock flow. RPCs are sent to the firmware's
`tokyo2006__cirque` subsystem (see `../proto/tokyo2006/cirque/cirque.proto`).

## Run the dev server

```bash
cd web
npm install            # one-time
npm run generate       # generate TypeScript types from ../proto (requires buf)
npm run dev            # http://localhost:5173/
```

`npm run dev` runs `buf generate` first so the generated proto types stay
fresh, then starts Vite. Web Serial / Web Bluetooth require a secure
context (HTTPS or `localhost`) and a Chromium-based browser.

## Run the checks

```bash
npm run lint           # eslint + prettier
npm test               # jest
npm run build          # tsc -b + vite build (needs npm run generate first)
npm run e2e            # playwright -- real firmware in Renode, no hardware
```

## Publish / deploy

The hosted UI lives at
`https://tokyo2006.github.io/cirque-input-module/`. Pushing to `main`
(or `main+custom-studio-protocol`) on this repo triggers
`.github/workflows/web-ui.yml`, which:

1. Runs `npm ci` and `npm run generate`.
2. Runs `npm run lint`, `npm test`, and `npm run build` (the build is
   invoked with `VITE_BASE=/cirque-input-module/` so asset paths work
   under the GitHub Pages sub-path).
3. Uploads `web/dist/` to the `github-pages` environment, where
   `actions/deploy-pages@v4` publishes it.

Pull requests get a Cloudflare Workers preview when the
`CLOUDFLARE_API_TOKEN` and `CLOUDFLARE_ACCOUNT_ID` repo secrets are
set; otherwise the workflow posts a notice explaining the gap and
skips the deploy.

If you fork this repo and want the same GitHub Pages URL pattern,
either keep the repo named `cirque-input-module` (so
`github.event.repository.name` matches the `VITE_BASE` literal) or
change both the literal here in `vite.config.ts` and the
`VITE_BASE` arg in `.github/workflows/web-ui.yml`.

## Where the code lives

```
src/
├── main.tsx              React entry point
├── App.tsx               Connection UI + RPC test section
├── App.css               Styles
└── proto/                Generated protobuf TypeScript types (gitignored)
    └── tokyo2006/cirque/cirque.ts

test/
├── App.spec.tsx              Unit tests for the connection UI
└── RPCTestSection.spec.tsx   Unit tests for the RPC + Studio unlock flow

e2e/                      Playwright specs (real firmware in Renode)
```
