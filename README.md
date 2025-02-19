# Ilya Programming Language
## THIS PROJECT WILL NO LONGER BE UPDATED, AS DEVELOPMENT HAS MOVED TO [VMK](https://github.com/nginrsw/vmk-lang)

<img src="img/ilya.png" alt="Ilya" width="200"/>

Ilya is a Lua forked / Lua dialect Programming language, This modification has
been made to simplify learning, writing, and personalizing the language for my
own use.

For now, two main changes have been implemented:

- The reserved word `local` has been replaced with `lock`.
- The reserved word `function` has been replaced with `fn`.

Other than these changes, everything remains identical to the original Lua
language.

---

## Installation

1. Clone the repository:
   ```bash
   git clone https://github.com/nginrsw/ilya-lang-lang.git
   cd ilya
   ```

2. Build the program by running `make` in your terminal:
   ```bash
   make
   ```

   Or, if you want to test whether everything is working correctly, you can run
   the `all` file by executing:
   ```bash
   ./all
   ```

3. After building, create a symbolic link to install the program:
   ```bash
   sudo ln -s /path/to/program/ilya-lang/ilya /usr/local/bin/ilya
   ```

   Replace `/path/to/program` with the actual path to where the compiled program
   resides.

Once this is done, you can run the `ilya` command from anywhere in your
terminal!

For example, create a file named `file.ilya`, type `io.write("Hello Ilya\n")`
inside it, and run it with the following command:

```bash
ilya file.ilya
```

---

## Transpiling Between Lua and Ilya

If you wish to transpile code from Lua to Ilya or from Ilya to Lua, I’ve
prepared executable files located in the `transpiler` folder.

You can easily run these using the following commands:

- **From Lua to Ilya**, just run `lua2ilya` inside current directory:
  ```bash
  ./lua2ilya
  ```

- **From Ilya to Lua**, it's similar, run `ilya2lua` inside of ur current
  directory:
  ```bash
  ./ilya2lua
  ```

These commands will automatically convert all of your code between the two
languages, making it easier to switch between them.
