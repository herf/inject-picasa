## What's this about?

Well, it makes Picasa faster on NAS (samba) drives. It's been twenty years since I tried to make this case fast, and some people (including me) still use the app on Windows.

### tl;dr how do I use it?

Clone this repo then run this:

> powershell -ExecutionPolicy Bypass -File .\\install.ps1

What it does is install a small DLL next to Picasa3.exe that will change buffering options when files over a network drive are opened, which makes it about 4x faster.

### More details please?

I upgraded my network recently to 2.5gbps and Picasa didn't get any faster with the huge number of files I have on my NAS, and I wondered why. Procmon reminded me that Picasa was reading files using 64kb "unbuffered" reads. This is due to two things: first, we were doing pretty when we optimized things in 2001, because the defaults back then were like 4 or 8k per read, and we realized we could go faster by reading 64kb at a time - this was the max supported at the time (by stdio), I think. But then, and this second step was the issue - we also decided that background reads should not wipe out the OS buffer cache, so we set them to unbuffered. Which was a good idea for local disks, and not so much for networked ones.

The issue with reading 64kb at a time over the network is latency, and if you have a 1-5ms ping to your NAS, this single read has to complete before a new one is issued. When you add in a disk seek, you'll likely not get better than 300mbps over a LAN segment, and less than 100mbps over Wi-Fi. 

**So the only answer is to read more at a time** 

But the Picasa code hasn't been changed in a long time - it would be a lot of surgery to allocate larger buffers - so what can we do?

Well, the Windows OS anticipated all this, and it will do read-ahead sometimes (reading larger blocks). But, we broke it, because when you set a file handle to unbuffered, it turns this off. Oh! 

So what we do here is pretty simple: we make a DLL that hooks file opens, sets them to unbuffered if they are over a network, and Windows then reads 256kb at a time. That's it!

### The code

Claude wrote the proxy code - it suggested shim'ing out version.dll to do it (because version doesn't have that many functions), so the install process puts our fake CreateFile function ahead of the real one, and that's the whole trick. Windows isn't too strict about code-signing but there's a chance you will need to exclude your antivirus here. Hope it helps.
