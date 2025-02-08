#include "utilities.h"

enum class OrderType
{
    GoodTillCancel,
    FillAndKill
};

enum class Side
{
    Buy,
    Sell  
};

using Price = std::int32_t;
using Quantity = std::uint32_t;
using OrderId = std::uint64_t;

struct LevelInfo
{
    Price price_;
    Quantity quantity_;
};

using LevelInfos = std::vector<LevelInfo>;

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

class Order
{
    private:
    OrderType orderType_;
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity initialQuantity_, remainingQuantity_;

    public:
    Order(OrderType orderType, OrderId orderId, Side side, Price price, Quantity quantity)
        : orderType_(orderType)
        , orderId_(orderId)
        , side_(side)
        , price_(price)
        , initialQuantity_(quantity)
        , remainingQuantity_(quantity)
        {}

    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const {return side_; }
    Price GetPrice() const {return price_; }
    OrderType GetOrderType() const {return orderType_; }
    Quantity GetInitialQuantity() const {return initialQuantity_; }
    Quantity GetRemainingQuantity() const {return remainingQuantity_; }
    Quantity GetFilledQuantity() const {return GetInitialQuantity() - GetRemainingQuantity(); }
    bool isFilled() const { return GetRemainingQuantity() == 0; }
    void Fill(Quantity quantity)
    {
        if (quantity > GetRemainingQuantity())
        {
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", GetOrderId()));
        }
        remainingQuantity_ -= quantity;
    }
};

using OrderPointer = std::shared_ptr<Order>;
using OrderPointers = std::list<OrderPointer>;

class OrderModify
{
    private:
    OrderId orderId_;
    Side side_;
    Price price_;
    Quantity quantity_;

    public:
    OrderModify(OrderId orderId, Side side, Price price, Quantity quantity)
        : orderId_(orderId)
        , side_(side)
        , price_(price)
        , quantity_(quantity)
        {}
    
    OrderId GetOrderId() const { return orderId_; }
    Side GetSide() const { return side_; }
    Price GetPrice() const {return price_; }
    Quantity GetQuantity() const { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, GetOrderId(), GetSide(), GetPrice(), GetQuantity());
    } 
};

struct TradeInfo
{
    OrderId orderId_;
    Price price_;
    Quantity quantity_;  
};

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
        const auto& [existingOrder, _] = orders_.at(order.GetPrice());
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

        // auto CreateLevelInfos = [] (Price price, const OrderPointers& orders)
        // {
        //     return LevelInfo(price, std::accumulate(orders.begin(), orders.end(), (Quantity)(0),
        //             [](std::size_t runningSum, const OrderPointer& order)
        //             {
        //                 return runningSum + order -> GetRemainingQuantity();
        //             }
        // };
    }
};

int main()
{
    
}