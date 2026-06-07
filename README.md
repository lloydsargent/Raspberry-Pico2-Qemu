# Raspberry-Pico2-Qemu
This is a Raspberry Pico2 Qemu. 

## What to Expect

Basically, it acts a lot like the Raspberry Pi RP2350 which is the Arm Cortex-M33 and is based off the Cortex-M33 that is part the QEMU base code. The real change in this is that it uses the memory sizes for the Pico-2.

> ⚠️ **You have been warned**  I really don't know the insides of the Qemu architecture.
> 
> That is not me being modest, it really is a big hack.
> 
> So if it fails, good luck, I can guarantee I do not know how to fix it.
> 
> That said, I've been using it for about four months and have yet to crash it. If it breaks, pull the plug, count to ten, then plug it back in. That is the best support you will get.

## What to expect

It will work until it fails. Again, that's not me being snarky, that's me being ignorant about the guts of Qemu. If someone who knows how Qemu works, feel free to send me the mods to make it better. I guarantee I will have absolutely no idea what you did, but will be impressed.

## Okay, I've read all the warnings and don't care. How do I build it. 

This assumes you are using the command line. There are no other instructions. Not even an executable. Honestly, it's not that difficult (not as difficult as hacking Qemu which, did I mention it, I really don't understand). It's only seven steps. Easy peasy.

> BTW, this all depends on you making sure you followed Qemu's dependencies which I'm not goint to repeat here, because they do a **much** better job)

* Step 1

Create a directory and name it qemu (or whatever name you want to call it). `cd` into that directory. Now run the following.

```
git clone https://github.com/qemu/qemu.git
```

* Step 2

Download the three files (in my repo), `rp2350.c`, `Kconfig`, and `meson.build` into a different folder. Originally I was going to suggest you copy them over the originals, but that won't work. 

> Seriously, do not copy them over the originals. They are there for reference. Someone else suggested copying them over the originals and it just didn't work. `Kconfig` and `meson.build` change a lot. Did I mention I am not a Qemu expert (or even novice) so it will probably break things. And you can count on me not to be able to help you.

* Step 3

Move `rp2350.c` into `qemu/hw/arm` folder. That's it for that file.

* Step 4

Do **NOT** just copy `Kconfig` and `meson.build` over the original ones. You probably will get build errors. Also, I don't know how to fix it because I don't. This was all hacked together. Someone else suggested "copy them over" and it just didn't work. ⚠️ **You have been warned**

* Step 5

Add the following lines in Kconfig:

```
config RP2350
    bool
    default y
    depends on TCG && ARM
    select ARMSSE
    select PL011 # UART
    select PL031
    select SPLIT_IRQ
    select UNIMP
```

This allows Kconfig to know about the RP2350

* Step 6

Edit `meson.build`. Under this line:

```
arm_common_ss.add(when: 'CONFIG_MUSCA', if_true: files('musca.c'))
```

Add the following line:

```
arm_common_ss.add(when: 'CONFIG_RP2350', if_true: files('rp2350.c'))
```

* Step 7

Now, you should be able to type the following (assuming all of your other dependencies are satisfied -- go to the QEMU repo to find out what those may be)

```
mkdir build
cd build
../configure --target-list=arm-softmmu -machine=rp2350-pico2 --cpu=cortex-m33
make
```

You may get some warnings, but they can safely be ignored. At the end of all this, you should get a file:

`qemu/build/qemu-system-arm-unsigned` which is the executable.

Type the following:
```
./qemu-system-arm-unsigned -cpu cortex-m33 -machine rp2350-pico2 -nographic
```

And you should get the following:

```
 Welcome to the RP2350 Pico-2 Emulator
QEMU 11.0.50 monitor - type 'help' for more information
(qemu) qemu: fatal: Lockup: can't escalate 3 to HardFault (current priority -1)

R00=00000000 R01=00000000 R02=00000000 R03=00000000
R04=00000000 R05=00000000 R06=00000000 R07=00000000
R08=00000000 R09=00000000 R10=00000000 R11=00000000
R12=00000000 R13=ffffffe0 R14=fffffff9 R15=00000000
XPSR=40000003 -Z-- A S handler
s00=00000000 s01=00000000 d00=0000000000000000
s02=00000000 s03=00000000 d01=0000000000000000
s04=00000000 s05=00000000 d02=0000000000000000
s06=00000000 s07=00000000 d03=0000000000000000
s08=00000000 s09=00000000 d04=0000000000000000
s10=00000000 s11=00000000 d05=0000000000000000
s12=00000000 s13=00000000 d06=0000000000000000
s14=00000000 s15=00000000 d07=0000000000000000
s16=00000000 s17=00000000 d08=0000000000000000
s18=00000000 s19=00000000 d09=0000000000000000
s20=00000000 s21=00000000 d10=0000000000000000
s22=00000000 s23=00000000 d11=0000000000000000
s24=00000000 s25=00000000 d12=0000000000000000
s26=00000000 s27=00000000 d13=0000000000000000
s28=00000000 s29=00000000 d14=0000000000000000
s30=00000000 s31=00000000 d15=0000000000000000
FPSCR: 00000000
zsh: abort      ./qemu-system-arm-unsigned -cpu cortex-m33 -machine rp2350-pico2 -nographic
```

# That should be it!

Feel free to leave an issue, but I can pretty much tell you I have zero knowledge on how to fix the problem. If worse comes to worse, start from the beginning of the instructions and do them very slowly. I tried to be comprehensive, but sometimes I make typeing errors.

> ❗️ Did I mention my understanding of QEMU internals is near nil? It really is! But so far, this project seems to work with QEMU 11.0.50, so if you are having trouble, you may want to pull that version rather than the default one. After all, you just want a Pico2 emulator, right?
