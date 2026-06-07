*** **IN PROGRESS** ***


# Raspberry-Pico2-Qemu
This is a Raspberry Pico2 Qemu. 

# What to Expect

Basically, it acts a lot like the Raspberry Pi RP2350 which is the Arm Cortex-M33 and is based off the Cortex-M33 that is part the QEMU base code. The real change in this is that it uses the memory sizes for the Pico-2.

# BIG WARNING

I really don't know the insides of the Qemu architecture. It really is a big hack. So if it fails, good luck, I can guarantee I do not know how to fix it. That said, I've been using it for about four months and have yet to have it crash on me. That doesn't mean it works, just that I haven't hit an edge case where it fails.

# What to expect

It will work until it fails.

# Okay, I've read all the warnings and don't care. How do I build it

This assumes you are using the command line. There is no other instruction. 

* Step 1

Create a directory and name it qemu (or whatever name you want to call it). `cd` into that directory. Now run the following.

```
git clone https://github.com/qemu/qemu.git
```

* Step 2


