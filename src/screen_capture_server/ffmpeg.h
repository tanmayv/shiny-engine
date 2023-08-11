#include <absl/status/statusor.h>
#include <span>
#include <type_traits>
#include <vector>
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
}

// template <typename T, typename... Types>
// void CreatePipeline()
// {

// };


// class Decoder {
// public:
//   char *Process(char *);
// };

// class FramesReader {
// public:
//   char *Process(std::string filename);
// };

// auto display = [](char *) -> void {};


// CreatePipeline<Decoder, FramesReader>();


template<typename... Ts>
struct select_last {
  template<typename T>
  struct tag {
    using type = T;
  };
  // Use a fold-expression to fold the comma operator over the parameter pack.
  using type = typename decltype((tag<Ts>{}, ...))::type;
};

template<typename... Ts>
using select_last_t = typename select_last<Ts...>::type;


template<typename param, typename Last, typename... Rest>
struct return_type {
  using type = std::invoke_result_t<decltype(&Last::Step), param>;
};

template<typename param, typename First, typename Next, typename... Rest>
struct return_type<param, First, Next, Rest...> {
  using temp_type = std::invoke_result_t<decltype(&First::Step), param>;
  using type = return_type<temp_type, Next, Rest...>;
};

template<typename... Ts>
using return_type_t = typename return_type<Ts...>::type;

class MultiplyBy {
public:
  int Step(int x) { return 11 * x; }
};

class AddOn {
public:
  int Step(int x) { return x + 5; }
};

// template<typename StartParam, typename FirstStage, typename... Stages>
// class Pipeline {
// public:
//   void Add(std::variant<FirstStage, Stages...> stage) { stages.push_back(std::move(stage)); }

//   return_type_t<StartParam, FirstStage, Stages...> Execute(StartParam) {
//     return 0;
//   }
//   return_type_t<StartParam, FirstStage, Stages...> Execute() {
//     return 0;
//   }

// private:
//   std::vector<std::variant<FirstStage, Stages...>> stages;
// };

template<typename ParamType, typename T>
struct step_return_type {
  using type = decltype(std::declval<T &>().Step(std::declval<ParamType>()));
};

template<typename T>
struct step_return_type<void, T> {
  using type = decltype(std::declval<T &>().Step());
};

// struct MyTest<T, void> {
//   using type = decltype(std::declval<T &>().Step());
// };

// template<typename ParamType, typename T>
// struct MyTest<ParamType, T> {
//   using type = decltype(std::declval<First&>().Step(ParamType));
// };

// template<typename Callable>
// using return_type_of_t =
//     typename decltype(std::function{std::declval<Callable>()})::result_type;

// template<typename... Stages>
// struct pipeline_result;

// // atleast 2
// template<typename ParamType, typename Stage, typename... Remaining>
// struct pipeline_result<ParamType, Stage, Remaining...> {
//   using type = typename pipeline_result<typename step_return_type<ParamType, Stage>::type, Remaining...>::type;
// };

// last type left
// template<typename ParamType>
// struct pipeline_result<ParamType> {
//   using type = ParamType;
// };

// Pipeline class which takes pointers to each class and call step function on each one.

// using pack_t = typename pack::type;

// template<typename variant_type, typename ParamType, typename... Stages>
// typename pipeline_result<ParamType, Stages...>::type Executor() {
//   return Executor<variant_type, >
// }

// template<typename variant_type, typename ParamType, typename... Stages>
// typename pipeline_result<ParamType, Stages...>::type Executor<ParamType, Stages...>(*... stages, ParamType result) {

// };

// template<typename ParamType>
// ParamType Executor(ParamType result) {
//   return result;
// }
// template<class... Fs>
// class Functions {
//   std::tuple<Fs...> m_functions;

//   template<size_t index, class Arg>
//   decltype(auto) CallHelper(Arg &&arg) {
//     if constexpr (index == 0) {
//       return std::forward<Arg>(arg);
//     } else {
//       return std::get<index - 1>(m_functions)(CallHelper<index - 1>(std::forward<Arg>(arg)));
//     }
//   }

// public:
//   Functions(Fs... functions) : m_functions(functions...) {}

//   template<class Arg>
//   decltype(auto) operator()(Arg &&arg) {
//     return CallHelper<sizeof...(Fs)>(std::forward<Arg>(arg));
//   }
// };

template<typename BaseArg, typename... Stages>
class Pipeline {
public:
  Pipeline(Stages *...stages) : stages_(stages...) {}

private:
  template<size_t index, class Arg>
  decltype(auto) Executor(Arg &&arg) {
    if constexpr (index == 0) {
      return std::forward<Arg>(arg);
    } else {
      return std::get<index - 1>(stages_)->Step(Executor<index - 1>(std::forward<Arg>(arg)));
    }
  }

public:
  decltype(auto) Execute(BaseArg &&arg) { return Executor<sizeof...(Stages)>(arg); }

private:
  std::tuple<Stages *...> stages_;
};

void Test() {
  auto f = std::make_unique<MultiplyBy>();
  auto s = std::make_unique<AddOn>();

  // static_assert(std::is_same_v<pipeline_result<void, First, Second>::type, int>);
  // pipeline_result<void, First, Second>::type x = 100;
  Pipeline<int, MultiplyBy, AddOn> p(f.get(), s.get());
  std::cout << "Result: " << p.Execute(5);

  // step_return_type<int, Second>::type x = 100;
  // step_return_type<void, First>::type xy= 100;
  // select_last_t<int,bool> b = true;
  // Pipeline<void, First, Second> pipeline;
  // First f;
  // Second s;
  // pipeline.Add(f);
  // pipeline.Add(s);
  // pipeline.Execute();
}