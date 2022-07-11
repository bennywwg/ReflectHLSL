In the context of parsers, programs which scan strings of text and perform actions based on the content of that text, a *Grammar* is defined by a set of production rules. This grammar then defines a *language*.  Grammar Production Rules take the following form:

`T`<sub>`Product`</sub> `|=` `T`<sub>`2`</sub> `T`<sub>`3`</sub> `...`  `T`<sub>`n`</sub>

When parsing text, the text is broken up into a string of *nonterminals*, denoted here as `T`<sub>`n`</sub>. The parser proceeds along the string, and when it encounters the sequence of nonterminals on the right hand side, it may convert that sequence into the left hand side.

With that in mind, I will turn to `parsegen-cpp`: This is a great little parser project that does exactly what I wrote above. It is a C++ library used to define grammars using production rules.  Now before I enter criticism mode, I want to give credit where it is due: the style of `parsegen-cpp` is extremely unopinioated, and it isn't designed *poorly*. Every part of it was eminently understandable and every aspect felt intuitive. For example, defining a parsegen language could not be simpler:

```
struct language {
  struct token {
    std::string name;
    std::string regex;
  };
  std::vector<token> tokens;
  struct production {
    std::string lhs;
    std::vector<std::string> rhs;
  };
  std::vector<production> productions;
};
```

So defining a grammar could look something like this:

```
language lang;
lang.tokens = {
    token { "A_Token", "A" }
};
lang.productions = {
    production { "Root_Nonterminal", { "A_List" } },
    production { "A_List", { "A_Token" } },
    production { "A_List", { "A_List", "A_Token" } }
};
```

This language would accepts any nonzero number of A's in a string. Seems pretty good, right? And for simply making a program that will either accept or reject a given input string, its probably nearly impossible to improve on this. Unfortunately, accepting and rejecting a string isn't the only task of a parser. To do anything interesting, the parser needs to invoke behavior when a production rule occurs. And here, in my opinion, is where `parsegen-cpp` falls apart from a usability and api design perspective.

What if we want to count the number of A's by incrementing a global variable any time either of "A_List" production rules is invoked? `parsegen-cpp` offers a virtual function that can be overridden to do just this, with the following signature:
  inline virtual std::any reduce(int prod, std::vector<std::any>& rhs)

This function is called every time a rule is invoked. Here is where the first major usability problem occurs, in my opinion: the function parameter `prod` is an index into lang.productions, which represents which production rule is occurring. Oh boy, where do I even begin. Well, this technically does offer all the information needed to accomplish our task. We could simple override the function to be:
```
inline virtual std::any reduce(int prod, std::vector<std::any>& rhs) override {
  if (prod >= 1 && prod <= 2) {
    global_A_count++;
  }
}
```

The usability problem with this is that if the productions table were ever to be changed, the indices of the rules would also change, which means any time the production rules change, we also need to change the overriden reduce function. This is simply unacceptable. The second major problem, which is much worse in my opinion, is that the use of `std::any` to transfer information between nonterminals means any time information is retrieved during a production rule, a programmer needs to mentally keep track of which types will be present in the any vector. This is also unacceptable. Clearly, the functionality exposed by `parsegen-cpp` is not suitable to being used directly to develop a parser, however as I will show below, it can be used as the foundation for a truly usable system that eliminates both of these problems.

How We Can Make Improvements

Imagine the following association between grammar concepts and C++ concepts:
    `production rule <-> function`
    `nonterminal <-> type`

A C++ function looks like this:

`ReturnType FunctionName(Param0 p0, Param1 p1, ..., ParamN pN) { ... }`
or
`[](Param0 p0, Param1 p1, ..., ParamN pN) -> ReturnType { ... }`

This looks eerily similar to a production rule:

`struct production { std::string lhs; std::vector<std::string> rhs; };`

What if there was a way to convert functions into productions, for example, the function:

`[](Param0 p0, Param1 p1, ..., ParamN pN) -> ReturnType { ... }`

would become:

`production { "ReturnType", { "Param0", "Param1", ..., "ParamN" } }`

But is this possible? Using C++ typeid operator, we can in fact get the string associated with a certain type. For example, typeid(int).name() will return the string "int". Or at least, it is guaranteed to return a unique string associated with that type. But that is not enough, in fact we need to create a vector<string> of the unique names of each the types of each parameter for the function. This can be accomplished using recursive variadic templates and SFINAE:

```
template<typename ...Args>
typename std::enable_if<sizeof...(Args) == 0, void>::type
RegTypes(vector<string>&) { };

template<typename R, typename ...Args>
typename std::enable_if<sizeof...(Args) == 0, void>::type
RegTypes(vector<string>& res) {
    res.back() = typeid(R).name();
};

template<typename R, typename ...Args>
typename std::enable_if<sizeof...(Args) != 0, void>::type
RegTypes(vector<string>& res) {
    RegTypes<Args...>(res);
    res[res.size() - sizeof...(Args) - 1] = typeid(R).name();
};
```

Here's an example of how this function in practice:

```
vector<string> names;
names.resize(4);
RegTypes<int, float, std::string, double>(names);
// names == { "int", "float", "std::string", "double" }
```

Now let me show you what this enables. Since we can convert a list of types into a string of their names, we have all the necessary ingredients to make a production rule! Here's an example function that could do that:

```
template<typename R, typename ...Args>
production MakeProduction(function<R(Args...)> func) {
  // Get the result typename
  string lhs = typeid(R).name();

  // Now, get the list of type names used to produce the result typename
  vector<strings> rhs;
  rhs.resize(sizeof...(Args));
  RegTypes<Args...>(rhs);

  return production { lhs, rhs };
}
```

And we can use this function to automatically create production rule from a function or lambda, like this example which converts two ints into a bool, for example maybe like an equality operator:

```
// The template arguments are automatically deduced!
MakeProduction([](int firstOperand, int secondOperand) -> bool {
    return firstOperand == secondOperand;
});
```
