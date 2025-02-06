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
    OrderbookLevelInfos(const LevelInfos &bids, const LevelInfos &asks) : bids_(bids), asks_(asks)
    {}

    const LevelInfos& getBids() { return bids_; }
    const LevelInfos& getAsks() { return asks_; }
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

    OrderId getOrderId() const { return orderId_; }
    Side getSide() const {return side_; }
    Price getPrice() const {return price_; }
    OrderType getOrderType() const {return orderType_; }
    Quantity getInitialQuantity() const {return initialQuantity_; }
    Quantity getRemainingQuantity() const {return remainingQuantity_; }
    Quantity getFilledQuantity() const {return getInitialQuantity() - getRemainingQuantity(); }
    void Fill(Quantity quantity)
    {
        if (quantity > getRemainingQuantity())
        {
            throw std::logic_error(std::format("Order ({}) cannot be filled for more than its remaining quantity.", getOrderId()));
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
    
    OrderId getOrderId() const { return orderId_; }
    Side getSide() const { return side_; }
    Price getPrice() const {return price_; }
    Quantity getQuantity() const { return quantity_; }

    OrderPointer ToOrderPointer(OrderType type) const
    {
        return std::make_shared<Order>(type, getOrderId(), getSide(), getPrice(), getQuantity());
    } 
};

int main()
{
    
}