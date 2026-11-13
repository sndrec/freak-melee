function validUrl(value) {
  if (new TextEncoder().encode(value).length > 2048 || /[\s<>"'\\]/u.test(value)) return false;
  try {
    const url = new URL(value);
    return ["https:", "http:"].includes(url.protocol) && !!url.hostname && !url.username && !url.password;
  } catch { return false; }
}

function appendInline(parent, source) {
  let offset = 0, plain = "";
  function flush() { if (plain) parent.append(document.createTextNode(plain)); plain = ""; }
  while (offset < source.length) {
    let label = "", url = "", end = offset;
    if (source[offset] === "[") {
      const close = source.indexOf("](", offset + 1);
      if (close > offset + 1 && !source.slice(offset, close).includes("\n")) {
        let cursor = close + 2, depth = 1;
        while (cursor < source.length && !/[\s<>"']/u.test(source[cursor])) {
          if (source[cursor] === "(") depth++;
          if (source[cursor] === ")" && --depth === 0) break;
          cursor++;
        }
        const candidate = source.slice(close + 2, cursor);
        if (!depth && validUrl(candidate)) {
          label = source.slice(offset + 1, close); url = candidate; end = cursor + 1;
        }
      }
    }
    if (end === offset && /^https?:\/\//iu.test(source.slice(offset)) &&
        (!offset || !/[a-z0-9]/iu.test(source[offset-1]))) {
      let cursor = offset, parentheses = 0, brackets = 0;
      while (cursor < source.length && !/[\s<>"']/u.test(source[cursor])) {
        const c = source[cursor];
        if (c === "(") parentheses++;
        if (c === ")" && --parentheses < 0) break;
        if (c === "[") brackets++;
        if (c === "]" && --brackets < 0) break;
        cursor++;
      }
      const candidate = source.slice(offset, cursor).replace(/[.,;!]+$/u, "");
      if (validUrl(candidate)) { label = url = candidate; end = offset + candidate.length; }
    }
    if (end > offset) {
      flush();
      const link = document.createElement("a");
      link.textContent = label; link.href = url; link.target = "_blank"; link.rel = "noopener noreferrer";
      parent.append(link); offset = end;
    } else plain += source[offset++];
  }
  flush();
}

export function formatNews(parent, body) {
  let paragraph = null, list = null;
  for (const line of body.split("\n")) {
    if (!line.trim()) { paragraph = list = null; continue; }
    const heading = /^(#{1,3}) (.*)$/u.exec(line);
    if (heading) {
      const element = document.createElement(`h${heading[1].length+1}`);
      appendInline(element, heading[2]); parent.append(element); paragraph = list = null;
    } else if (line.startsWith("- ")) {
      if (!list) { list = document.createElement("ul"); parent.append(list); }
      const item = document.createElement("li"); appendInline(item, line.slice(2)); list.append(item); paragraph = null;
    } else {
      if (!paragraph) { paragraph = document.createElement("p"); parent.append(paragraph); }
      else paragraph.append(document.createTextNode("\n"));
      appendInline(paragraph, line); list = null;
    }
  }
}
