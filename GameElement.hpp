//
// Created by kovi on 2/15/17.
//

#ifndef SHADOW_GAMEELEMENT_HPP
#define SHADOW_GAMEELEMENT_HPP

class GameElement {
public:
    virtual void init() = 0;

    virtual void render() = 0;
};

#endif //SHADOW_GAMEELEMENT_HPP
