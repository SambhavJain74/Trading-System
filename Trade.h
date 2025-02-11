#pragma once

#include "TradeInfo.h"

class Trade 
{
    private:
    TradeInfo bidTrade_, askTrade_;

    public:
    Trade(const TradeInfo &bidTrade, const TradeInfo &askTrade)
    : bidTrade_(bidTrade) 
    , askTrade_(askTrade)
    {}

    const TradeInfo& GetBidTrade() { return bidTrade_; }
    const TradeInfo& GetAskTrade() { return askTrade_; }
};

using Trades = std::vector<Trade>;