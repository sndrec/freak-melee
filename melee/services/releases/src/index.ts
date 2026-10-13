const archiveName = /^freak-melee-(?:setup-)?[a-zA-Z0-9._-]{1,64}\.zip$/;

export default {
  async fetch(request: Request, env: CloudflareBindings): Promise<Response> {
    if (request.method !== "GET") {
      return new Response("Method not allowed", { status: 405 });
    }

    const path = new URL(request.url).pathname;
    const prefix = `/${env.CHANNEL_PATH}/`;
    if (!path.startsWith(prefix)) {
      return new Response("Not found", { status: 404 });
    }
    const key = path.slice(prefix.length);
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
