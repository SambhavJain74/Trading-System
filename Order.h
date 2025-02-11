#pragma once

#include "usings.h"

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