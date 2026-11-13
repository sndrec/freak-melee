import {afterEach, beforeEach, expect, it, vi} from "vitest";
import {formatNews} from "../public/admin/news-format.js";

function element(tag) {
  return {tag, children: [], append(...children) {this.children.push(...children);}};
}
function nodes(root, tag) {
  return [root, ...root.children.flatMap(child => nodes(child, tag))].filter(node => node.tag === tag);
}
function text(root) {return (root.textContent ?? root.value ?? "") + root.children.map(text).join("");}
beforeEach(() => vi.stubGlobal("document", {
  createElement: element,
  createTextNode: value => ({value, children: []}),
}));
afterEach(() => vi.unstubAllGlobals());

it("formats headings, lists, Markdown links and literal web URLs", () => {
  const root = element("root");
  formatNews(root, "# Help\n\nRead [the guide](https://example.com/docs_(test)?a=1&b=2).\n- Visit HTTP://example.com/x.");
  expect(nodes(root, "h2")).toHaveLength(1);
  expect(nodes(root, "li")).toHaveLength(1);
  const links = nodes(root, "a");
  expect(links.map(link => link.href)).toEqual(["https://example.com/docs_(test)?a=1&b=2", "HTTP://example.com/x"]);
  expect(links.map(link => link.textContent)).toEqual(["the guide", "HTTP://example.com/x"]);
  expect(links.every(link => link.target === "_blank" && link.rel === "noopener noreferrer")).toBe(true);
});
it("keeps HTML and unsafe schemes as text, including credential-bearing URLs", () => {
  const root = element("root");
  formatNews(root, "<img src=x onerror=alert(1)> [bad](javascript:alert(1)) [local](file:///x) https://user:pass@example.com/");
  expect(nodes(root, "a")).toHaveLength(0);
  expect(nodes(root, "img")).toHaveLength(0);
  expect(text(root)).toContain("<img src=x onerror=alert(1)>");
});
it("preserves Unicode labels and balanced URL punctuation", () => {
  const root = element("root");
  formatNews(root, "See (https://example.com/a_(b)). [日本語](https://example.com/日本語)");
  expect(nodes(root, "a").map(link => link.href)).toEqual(["https://example.com/a_(b)", "https://example.com/日本語"]);
  expect(text(root)).toBe("See (https://example.com/a_(b)). 日本語");
});
