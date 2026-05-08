# Seal
A scripting language that is beautiful, efficient and embeddable.

## Official Web Page
https://seallang.org

## Example
Seal has an elegant syntax that is easily readable and writable.
```php
person = {
    name = "John",
    age = 30,
}

define greet(name)
    print("Hello,", name)

greet(person.name) // Hello, John

person.talk = define(self) // first class functions
    print("My name is", self.name, "and I am", self.age, "years old") 

person->talk() // same as person.talk(person)

String.underscore = define(self)
    return "__" + self->upper() + "__"

print("seal"->underscore()) // __SEAL__
```

## Embedding
Since Seal is written in C, it is easily embeddable into any C/C++ applications.
```c
#include <seal.h>
#include <stdio.h>

static void seal_factor(seal_state *S)
{
    seal_checkargc(S, 1);
    int n = seal_checkint(S, 0);
    int result = 1;
    while (n > 1) {
        result *= n;
        n--;
    }
    seal_pushint(S, result);
}

int main(void)
{
    seal_state *S = seal_state_new();
    seal_register(S, seal_factor, "factor");
    seal_getglobal(S, "factor");
    seal_pushint(S, 5);
    seal_call(S, 1);
    printf("%lld\n", seal_toint(S, -1));
    seal_state_free(S);
    return 0;
}
```

## Install
```bash
git clone https://github.com/agayevhuseyn/seal
cd seal
make
./seal --version
```
