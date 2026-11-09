import { storeNetplayReport } from "./diagnostics";

const archiveName = /^freak-melee-(?:setup-)?[a-zA-Z0-9._-]{1,64}\.zip$/;
const reportPath = /^diagnostics\/([a-f0-9]{16})\/([0-3])$/;

export default {
  async fetch(request: Request, env: CloudflareBindings): Promise<Response> {
    const path = new URL(request.url).pathname;
    const prefix = `/${env.CHANNEL_PATH}/`;
    if (!path.startsWith(prefix)) {
      return new Response("Not found", { status: 404 });
    }
    const key = path.slice(prefix.length);
    const report = reportPath.exec(key);
    if (report !== null) return storeNetplayReport(request, env, report[1], report[2]);
    if (request.method !== "GET") return new Response("Method not allowed", { status: 405 });
    if (key !== "latest.json" && !archiveName.test(key)) {
      return new Response("Not found", { status: 404 });
    }

    const object = await env.RELEASES.get(key);
    if (object === null) {
      return new Response("Not found", { status: 404 });
    }

    return new Response(object.body, {
      headers: {
        "Content-Type": key === "latest.json"
          ? "application/json; charset=utf-8"
          : "application/zip",
        "Content-Length": String(object.size),
        "Cache-Control": key === "latest.json"
          ? "no-store"
          : "public, max-age=31536000, immutable",
      },
    });
  },
} satisfies ExportedHandler<CloudflareBindings>;
