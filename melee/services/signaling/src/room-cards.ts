export const NO_CARD = 255;
export type CardSlots = [number, number, number, number];

export function validCardSlots(slots: CardSlots, activeMask: number): boolean {
  if (!Array.isArray(slots) || slots.length !== 4) return false;
  let occupied = 0;
  for (let seat = 0; seat < 4; ++seat) {
    const card = slots[seat];
    if (!(activeMask & (1 << seat))) {
      if (card !== NO_CARD) return false;
      continue;
    }
    if (!Number.isInteger(card) || card < 0 || card >= 4 ||
        occupied & (1 << card)) return false;
    occupied |= 1 << card;
  }
  return true;
}

export function occupiedCards(people: { cardSlots: CardSlots }[]): number {
  let mask = 0;
  for (const { cardSlots } of people)
    for (const card of cardSlots)
      if (card < 4) mask |= 1 << card;
  return mask;
}

export function assignOpenCards(activeMask: number, unavailable: number,
  limit: number, previous?: CardSlots): CardSlots | null {
  const slots: CardSlots = [NO_CARD, NO_CARD, NO_CARD, NO_CARD];
  // Keep owned cards before assigning any newly opened local seats.
  for (let seat = 0; seat < 4; ++seat) {
    const card = previous?.[seat] ?? NO_CARD;
    if (!(activeMask & (1 << seat)) || card >= limit ||
        unavailable & (1 << card)) continue;
    slots[seat] = card;
    unavailable |= 1 << card;
  }
  for (let seat = 0; seat < 4; ++seat) {
    if (!(activeMask & (1 << seat)) || slots[seat] !== NO_CARD) continue;
    let card = 0;
    while (card < limit && (unavailable & (1 << card))) ++card;
    if (card === limit) return null;
    slots[seat] = card;
    unavailable |= 1 << card;
  }
  return slots;
}
