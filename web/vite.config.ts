import { defineConfig } from "vite";
import react from "@vitejs/plugin-react";

// https://vite.dev/config/
export default defineConfig({
  // The fallback base only matters for local dev/preview; CI sets VITE_BASE
  // to /<repo>/. Default is the deployed GitHub Pages path for this repo.
  base: process.env.VITE_BASE ?? "/cirque-input-module/",
  plugins: [react()],
});
