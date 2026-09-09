# External reference spritesheets

This POC is authored around two fighting-game sheets from The Spriters Resource:

- Kyo, *The King of Fighters R-2* (NGPC): https://www.spriters-resource.com/neo_geo_pocket/thekingoffightersr2/asset/572882/
- Iori, *The King of Fighters R-2* (NGPC): https://www.spriters-resource.com/neo_geo_pocket/thekingoffightersr2/asset/572880/
- Optional stage reference, Korea: https://www.spriters-resource.com/neo_geo_pocket/thekingoffightersr2/asset/474160/

These are copyrighted game assets used here for an educational learning project under the permissions applicable to this repository. Keep the source attribution when preparing or redistributing derived teaching assets.

The runtime expects each fighter to be normalized to 29 frames of 64x64 pixels, in this order:

1. idle: 4 frames
2. walk: 4 frames
3. crouch: 1 frame
4. jump: 2 frames
5. light attack: 3 frames
6. heavy attack: 4 frames
7. special attack: 5 frames
8. block: 1 frame
9. hit reaction: 2 frames
10. KO: 3 frames

Place the normalized frames in `assets/source/p1/000.png` ... `028.png` and `assets/source/p2/000.png` ... `028.png`. The preparation script packs them in a deterministic order.
