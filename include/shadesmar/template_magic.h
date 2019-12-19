//
// Created by squadrick on 18/12/19.
//

#ifndef SHADESMAR_TEMPLATE_MAGIC_H
#define SHADESMAR_TEMPLATE_MAGIC_H
namespace shm::template_magic {
template <class F, class... Args> void do_for(F fn, Args... args) {
  int _[] = {(fn(args), 0)...};
}

template <class...> struct types { using type = types; };
template <class Sig> struct args;
template <class R, class... Args> struct args<R(Args...)> : types<Args...> {};
template <class Sig> using args_t = typename args<Sig>::type;

} // namespace shm::template_magic
#endif // SHADESMAR_TEMPLATE_MAGIC_H
