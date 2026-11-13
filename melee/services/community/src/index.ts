import { requireAdmin } from "./auth";
import { adminNews, publicNews } from "./news";
import { adminFeedback, submitFeedback } from "./feedback";
import { HttpError, json } from "./http";
import { cleanAbandonedAttachments } from "./cleanup";

export default {
  async fetch(request: Request, env: CloudflareBindings): Promise<Response> {
    const url = new URL(request.url);
    try {
      if (url.pathname === "/health" && request.method === "GET")
        return json({ service: "freak-melee-community", status: "ok" });
      if ((url.pathname === "/" || url.pathname === "/admin") && request.method === "GET")
        return Response.redirect(new URL("/admin/", url).href, 302);
      if (url.pathname === "/v1/feedback") return await submitFeedback(request, env);
      if (url.pathname === "/v1/feed" || url.pathname === "/v1/news" || url.pathname.startsWith("/v1/news/") ||
          url.pathname === "/v1/announcements" || url.pathname.startsWith("/v1/announcements/"))
        return await publicNews(request, env);
      if (url.pathname.startsWith("/admin/")) {
        await requireAdmin(request, env);
        if (url.pathname.startsWith("/admin/api/feedback")) return await adminFeedback(request, env);
        if (url.pathname.startsWith("/admin/api/")) return await adminNews(request, env);
        const response = await env.ASSETS.fetch(request);
        const headers = new Headers(response.headers);
        headers.set("Cache-Control", "no-store");
        headers.set("X-Content-Type-Options", "nosniff");
        headers.set("Referrer-Policy", "same-origin");
        headers.set("Content-Security-Policy", "default-src 'self'; script-src 'self'; style-src 'self'; img-src 'self' blob:; connect-src 'self'; frame-ancestors 'none'; base-uri 'none'; form-action 'self'");
        return new Response(response.body, { status: response.status, headers });
      }
      throw new HttpError(404, "Page not found.");
    } catch (error) {
      if (error instanceof HttpError) return json({ error: error.message }, error.status);
      console.error(JSON.stringify({ message: "community request failed", path: url.pathname,
        error: error instanceof Error ? error.message : "Unknown error" }));
      return json({ error: "The service could not complete this request. Please retry." }, 500);
    }
  },
  async scheduled(_event: ScheduledController, env: CloudflareBindings): Promise<void> {
    await cleanAbandonedAttachments(env);
  },
} satisfies ExportedHandler<CloudflareBindings>;
