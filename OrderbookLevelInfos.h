#pragma once

#include "utilities.h"

class OrderbookLevelInfos
{
    private:
    LevelInfos bids_, asks_;
    
    public:
    OrderbookLevelInfos(const LevelInfos &bids, const LevelInfos &asks) 
    : bids_(bids)
    , asks_(asks)
    {}

    const LevelInfos& GetBids() { return bids_; }
    const LevelInfos& GetAsks() { return asks_; }
};