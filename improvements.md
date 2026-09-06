# conclusion
## kinda important
### 1. runnability (?)
I could not run this easily. The code is made in such a way that certain compiler args are necessary. 

Try to get rid of them to make it more portable.
### 2. formatting is inconsistent
I keep seeing instances where half of the functions are written one way while the other are written in another way.

Try making it more consistent by only using one formatting style throught the whole codebase
## less important
### 1. class indentation
Your classes currently look something like this:
```cpp
class Foo {

public:
    int Bar;
    ...

private:
    int SecretBar;
    ...
}

class Mars {

public:
    int Bar;
    ...

private:
    int SecretBar;
    ...
}
```
which makes them hard to read. You can only find out where you are writing either to (class-wise):
1. scrolling up to see
2. remembering

What would make it nicer is if they where indented, like this:
```cpp
class Foo {

    public:
        int Bar;
        ...

    private:
        int SecretBar;
        ...
}

class Mars {

    public:
        int Bar;
        ...

    private:
        int SecretBar;
        ...
}
```
### 2. structs/classes are inconsistent

Take the following examples:
```cpp
struct TopOfBook {

  int best_bid = 0;
  long long best_bid_quantity = 0;

  int best_ask = 0;
  long long best_ask_quantity = 0;
};
```
```cpp
struct Location_ID {

LevelNode *location;

Side side;

const int owner_id;

int price;
};
```
In one, the grouping is based off of function, in the other it is based off of type.

## end
Most of the stuff here is fine, but could be improved.

I haven't looked at everything yet, so might edit this at a later time.

Till then... g'bye