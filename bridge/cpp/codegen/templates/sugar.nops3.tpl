<!--(for op, opcode, _optype, _opcount, _typesigs, _layouts, broadcast in data)-->
template <typename TL, typename TR>
inline multi_array<TL>& @!op!@ (multi_array<TL>& lhs, multi_array<TR>& rhs)
{
    multi_array<TL>* left    = &lhs;
    multi_array<TR>* right   = &rhs;

    <!--(if broadcast)-->
    if (!same_shape(*left, *right)) {           // Broadcast
        left    = &Runtime::instance().temp_view(lhs);
        right   = &Runtime::instance().temp_view(rhs);

        if (lhs.getRank() < rhs.getRank()) {    // Left-handside has lowest rank
            if (!broadcast(*left, *right)) {
                throw std::runtime_error("Failed broadcasting.");
            }
        } else {                                // Right-handside has lowest rank
            if (!broadcast(*right, *left)) {
                throw std::runtime_error("Failed broadcasting.");
            }
        }
    }
    <!--(end)-->

    multi_array<TL>* res = &Runtime::instance().create_base<TL, TR>(*left); // Construct result
    @!opcode.lower()!@ (*res, *left, *right); // Encode and enqueue
    res->setTemp(true); // Mark res as temp

    return *res;
}

template <typename TL, typename TR>
inline multi_array<TL>& @!op!@ (multi_array<TL>& lhs, const TR rhs)
{
    multi_array<TL>* res = &Runtime::instance().create_base<TL, TL>(lhs); // Construct result
    @!opcode.lower()!@ (*res, lhs, rhs); // Encode and enqueue
    res->setTemp(true); // Mark result as temp

    return *res;
}

template <typename TL, typename TR>
inline multi_array<TL>& @!op!@ (const TL lhs, multi_array<TR>& rhs)
{
    multi_array<TL>* res = &Runtime::instance().create_base<TL, TR>(rhs); // Construct result
    @!opcode.lower()!@ (*res, lhs, rhs); // Encode and enqueue
    res->setTemp(true); // Mark result as temp

    return *res;
}
<!--(end)-->
