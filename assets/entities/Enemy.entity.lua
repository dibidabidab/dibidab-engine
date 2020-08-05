
components = {
    AABB = {
        halfSize = {5, 8}
    },
    Physics = {
        ignorePolyPlatforms = false
    },
    StaticCollider = {},
    Health = {
        takesDamageFrom = {"hit"},
        componentsToAddOnDeath = {
            SliceSpriteIntoPieces = {
                steps = 6
            }
        },
        currHealth = 2,
        maxHealth = 2,
        givePlayerArrowOnKill = "RainbowArrow"
    },
    AsepriteView = {
        sprite = "sprites/enemy"
    },
    LightPoint = {
        radius = 60
    }
}

