# The Savo Language

## Configuration Requirements

### Install the C compiler

In this readme I recommend downloading MinGW as a compiler which you can find at this <a href="https://www.phpcodewizard.it/mingw/mingw.zip">link</a> If you use Windows.
Next, again and only in Windows, extract, copy and paste the mingw folder in C:, update the PATH system variable(put: C:\mingw\bin)

For linux user:

Install MinGW with this command:
```bash
sudo apt-get install gcc.
```

### Parser

Now it's time to install a software that, together with the next one we will install, we are talking about Bison. Basically, Bison is a software that analyzes a stream of data, received from the command shell or a script, in order to recognize the grammar of a command or a construct of a programming language. Through Bison, we will generate the parser for our interpreter. Later we will see in detail how it works, now let's just install it. If you have a Windows system, you don't need to install anything, as Bison has already been integrated into the package you downloaded in the previous point. But if you have a Linux distribution, as was the case with the C compiler, all you have to do is type the following command from the terminal: 

```bash
sudo apt-get install bison
```

### The Lexical Analyzer

Earlier I talked about Bison as the active part together with another software, for the design of an interpreter. That other software is Flex. Flex is a lexical analyzer, i.e. a scanner generator, capable of analyzing a data stream, received from the command shell or from a script, in order to recognize the keywords and symbols, which make up our language, and perform a certain action every time a combination of characters, called TOKEN, is recognized. Don't worry, nothing complex, now let's just worry about installing Flex and then you will also understand how it works. Now, if you have a Windows system, you don't need to install anything, as Flex is already integrated, along with Bison and MinGW, in the package you downloaded earlier. But if you have a Linux distribution, as was the case with the C compiler and Bison, all you have to do is type the following command from the terminal: 

```bash
sudo apt-get install flex
```

## Use the Savo console

Now you can use the Savo language, go to the directory with you terminal, digit savo.exe and you can easily use the language

## Credits

Author: Dylan Patriarchi
Inspired by: <a href="https://www.phpcodewizard.it/antoniolamorgese">Antonio La Morgese</a>

## Contribute

If you want to contribute hit me on my personal <a href="mailto:dylanpat2004@icloud.com">mail</a>
