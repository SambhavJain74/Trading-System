#include "utilities.h"

class Orderbook 
{
    private:

    struct OrderEntry 
    {
        OrderPointer order_{nullptr};
        OrderPointers::iterator location_;
    };

    std::map<Price, OrderPointers, std::greater<Price>> bids_;
    std::map<Price, OrderPointers, std::less<Price>> asks_;
    std::unordered_map<OrderId, OrderEntry> orders_;

    bool CanMatch(Side side, Price price) const 
    {
        if (side == Side::Buy)
        {
            if (asks_.empty())
            {
                return false;
            }

            // const auto& [bestAsk, _] = *asks_.begin();
            // return price >= bestAsk;

            return (*asks_.begin()).first <= price;
        }
        else 
        {
            if (bids_.empty())
            {
                return false;
            }

            // const auto& [bestBid, _] = *bids_.begin();
            // return bestBid >= price;

            return (*bids_.begin()).first >= price;
        }
        return true;        // Control doesn't reach here anyways
    }

    Trades MatchOrders()
    {
        Trades trades;
        trades.reserve(orders_.size());
        while (!bids_.empty() && !asks_.empty())
        {
            auto& [bidPrice, bids] = *bids_.begin();
            auto& [askPrice, asks] = *asks_.begin();

            if (bidPrice <= askPrice)
            {
                break;
            }

            while (!bids.empty() && !asks.empty())
            {
                auto& bid = bids.front();
                auto &ask = asks.front();       // time price priority
                Quantity quantity = std::min(bid -> GetRemainingQuantity(), ask -> GetRemainingQuantity());
                bid -> Fill(quantity);
                ask -> Fill(quantity);
                
                if (bid -> isFilled())
                {
                    orders_.erase(bid -> GetOrderId());         // erase from orders_ before popping from bids because bid is being used by reference from bids, will cause Runtime Error otherwise
                    bids.pop_front();
                }

                if (ask -> isFilled())
                {
                    orders_.erase(ask -> GetOrderId());
                    asks.pop_front();
                }

                if (bids.empty())
                {
                    bids_.erase(bidPrice);
                }

                if (asks.empty())
                {
                    asks_.erase(askPrice);
                }

                trades.push_back(Trade
                    (TradeInfo(bid -> GetOrderId(), bid -> GetPrice(), bid -> GetRemainingQuantity())
                    ,TradeInfo(ask -> GetOrderId(), ask -> GetPrice(), ask -> GetRemainingQuantity())
                    ));
            }
        }

        if (!bids_.empty())
        {
            auto& [_, bids] = *bids_.begin();
            auto& order = bids.front();
            if (order -> GetOrderType() == OrderType::FillAndKill)
            {
                CancelOrder(order -> GetOrderId());
            }
        }

        if (!asks_.empty())
        {
            auto& [_, asks] = *asks_.begin();
            auto& order = asks.front();
            if (order -> GetOrderType() == OrderType::FillAndKill)
            {
                CancelOrder(order -> GetOrderId());
            }
        }

        return trades;
    }

    public:

    Trades AddOrder(OrderPointer order)
    {
        if (orders_.contains(order -> GetOrderId()))
        {
            return {};
        }

        if (order -> GetOrderType() == OrderType::FillAndKill && !CanMatch(order -> GetSide(), order -> GetPrice()))
        {
            return {};
        }

        OrderPointers::iterator iterator;

        if (order -> GetSide() == Side::Buy)
        {
            auto &orders = bids_[order -> GetPrice()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }
        else
        {
            auto &orders = asks_[order -> GetPrice()];
            orders.push_back(order);
            iterator = std::next(orders.begin(), orders.size() - 1);
        }
        orders_.insert({order -> GetOrderId(), OrderEntry(order, iterator)});
        return MatchOrders();
    }

    void CancelOrder(OrderId orderId)
    {
        if (!orders_.contains(orderId))
        {
            return;
        }
        const auto& [order, orderIterator] = orders_.at(orderId);
        if (order -> GetSide() == Side::Sell)
        {
            auto price = order -> GetPrice();
            auto& orders = asks_.at(price);
            orders.erase(orderIterator);
            if (orders.empty())
            {
                asks_.erase(price);
            }
        }
        else 
        {
            auto price = order -> GetPrice();
            auto &orders = bids_.at(price);
            orders.erase(orderIterator);
            if (orders.empty())
            {
                bids_.erase(price);
            }
        }
        orders_.erase(orderId);
    }

    Trades ModifyOrder(OrderModify order)
    {
        if (!orders_.contains(order.GetOrderId()))
        {
            return {};
        }
        const auto [existingOrder, _] = orders_.at(order.GetOrderId());
        CancelOrder(order.GetOrderId());
        return AddOrder(order.ToOrderPointer(existingOrder -> GetOrderType()));
    }

    std::size_t Size()
    {
        return orders_.size();
    }

    OrderbookLevelInfos GetOrderInfos() const 
    {
        LevelInfos bidInfos, askInfos;
        bidInfos.reserve(orders_.size());
        askInfos.reserve(orders_.size());

        auto CreateLevelInfos = [] (Price price, const OrderPointers& orders)
        {
            return LevelInfo(price, std::accumulate(orders.begin(), orders.end(), (Quantity)(0),
                    [](Quantity runningSum, const OrderPointer& order)
                    {
                        return runningSum + order -> GetRemainingQuantity();
                    } ));
        };

        for (const auto& [price, orders] : bids_)
        {
            bidInfos.push_back(CreateLevelInfos(price, orders));
        }

        for (const auto& [price, orders]: asks_)
        {
            askInfos.push_back(CreateLevelInfos(price, orders));
        }

        return OrderbookLevelInfos(bidInfos, askInfos);
    }
};

int main()
{
    Orderbook orderbook; 
    const OrderId orderId = 1;
    orderbook.AddOrder(std::make_shared<Order>(OrderType::GoodTillCanceled, orderId, Side::Buy, 100, 10));
    std::cout << orderbook.Size() << "\n";      // 1
    orderbook.ModifyOrder(OrderModify(orderId, Side::Buy, 100, 10));
    std::cout << orderbook.Size() << "\n";      // 1
    orderbook.CancelOrder(orderId); 
    std::cout << orderbook.Size() << "\n";      // 0
}