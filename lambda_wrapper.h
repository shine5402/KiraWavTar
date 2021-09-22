#ifndef LAMBDA_WRAPPER_H
#define LAMBDA_WRAPPER_H
/* Code from https://stackoverflow.com/questions/43720869/make-qtconcurrentmapped-work-with-lambdas
 * It provide a wrapper to lambda, make it capable to provide result_type, functor_type, arg_tuple, and arity.
 */
#include <utility>
#include <type_traits>

namespace lambda_wrapper_impl {
    template <class T> struct function_traits : function_traits<decltype(&T::operator())> {};

    template <typename ClassType, typename ReturnType, typename... Args>
    struct function_traits<ReturnType(ClassType::*)(Args...) const> {
       // specialization for pointers to member function
       using functor_type = ClassType;
       using result_type = ReturnType;
       using arg_tuple = std::tuple<Args...>;
       static constexpr auto arity = sizeof...(Args);
    };

    template <class Callable, class... Args>
    struct CallableWrapper : Callable, function_traits<Callable> {
       CallableWrapper(const Callable &f) : Callable(f) {}
       CallableWrapper(Callable &&f) : Callable(std::move(f)) {}
    };

    template <class F, std::size_t ... Is, class T>
    auto wrap_impl(F &&f, std::index_sequence<Is...>, T) {
       return CallableWrapper<F, typename T::result_type,
             std::tuple_element_t<Is, typename T::arg_tuple>...>(std::forward<F>(f));
    }
}


template <class F> auto lambda_wrapper(F &&f) {
   using traits = lambda_wrapper_impl::function_traits<F>;
   return lambda_wrapper_impl::wrap_impl(std::forward<F>(f),
                    std::make_index_sequence<traits::arity>{}, traits{});
}

#endif // LAMBDA_WRAPPER_H
