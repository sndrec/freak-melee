// Test identities are deterministic so the same simulated guest can reconnect.
export function guestHeaders(peer: string): Record<string, string> {
  const bytes = new TextEncoder().encode(peer);
  const key = Array.from({length:32}, (_, index) => bytes[index % bytes.length] ?? 0)
    .map(value => value.toString(16).padStart(2, "0")).join("");
  return { "X-Freak-Guest": key };
}
