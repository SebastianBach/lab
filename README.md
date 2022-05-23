Prototype of a "laboratory" that allows to prototype applications.

Arrange code blocks in a certain order:

```
Recipe recipe;

add_step(step::hello_world);
add_step_configure(step::set_values, conf::add_values::cnt, 10);
add_step(step::sum);
add_step(step::print);
```

Execute the resulting program:

```
run(recipe, ..., ..., ...);
```

Create the source code of an equivalent standalone command line application:

```
create_code(recipe, "my_app.cpp");
```
