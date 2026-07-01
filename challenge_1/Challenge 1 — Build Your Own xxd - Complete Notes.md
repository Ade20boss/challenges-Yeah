Everything i learned while building a hex dumper from scratch, in the order it came up. These are concepts notes

---

### 0. WHAT THE TOOL IS
`xxd` is a **hex dump** tool: It prints the raw bytes of a file in hexadecimal next to their ASCII representation. `cat` shows a file as text; `xxd` shows what is actually stored on disk, the actual bytes, including the invisible characters(newlines, tabs, etc) that the display hides.

The goal of the challenge: become `xxd`. Read any file, print three columns:

00000000: 2369 6e63 6c75 6465 203c 7374 6469 6f2e  #include <stdio.
└ offset ┘ └────────── bytes in hex ─────────────┘  └── ASCII gutter ──┘

### 1. THE CORE IDEA: INFORMATION IS BITS + CONTEXT
**The single most important realization in this whole chapter and its even in my journal of systems engineering: "raw bits", "a number", "a hex", and "a character" are not different things that convert into each other. They are essentially one value that's viewed in different contexts**

what physically exists:   00100011   (8 bits / 8 wires, each high or low)
read as binary        →   00100011
read as decimal       →   35
read as hex           →   23
read as ASCII         →   '#'

- The file never stored the character `#`. It stored the **number 35**(bit pattern `00100011`). `#` is just one way to read that number.
- There is no **conversion step** between these. You go straight from the byte to whatever representation you want to display.
- The meaning is decided by **what operation you perform**, not by the bits themselves. Add the bits → they're a number. Index memory with them → they're an address. Send to the screen → they're a character.

This is why the hex column and the ASCII column are just **two lenses on the same byte**

---

### 2. HEXADECIMAL(THE MIDDLE COLUMN)
- **A byte** = 8bits, holds a value 0 - 255.
- **Hex** = base 16. Digits `0 1 2 3 4 5 6 7 8 9 a b c d e f` (where a=10 … f=15).
- **One byte = exactly two hex digits** (`00` to `ff`). This is why hex is used for bytes: it lines up perfectly with the 8-bit boundary (4 bits per hex digit).
- **Example: #** = decimal 35 = hex `23`, because 2×16 + 3 = 35
- A byte splits into two **nibbles** (4-bit halves): high nibble + low nibble. `0x6e` = `0110 1110` → high nibble `0110` (=6), low nibble `1110` (=14) → `6e`

---
### 3. THE OFFSET(THE LEFT COLUMN)

- The offset is simply a **position counter** How many bytes into the file you are.
- It is not derived from the byte's value, the offset is just counting bytes: 0, 1, 2, 3, 4....
- Same concept as a **memory address** (CS:APP §1.4: "memory is a linear array of bytes, each with its own address, starting from 0"). A file is also a linear array of bytes.
- `xxd` prints 16 bytes per row, so it labels each row with the offset of its **first** byte: row 1 = 0, row 2 = 16, row 3 = 32 … (each row is +16).
- Those labels are written in **hex**, which is the only confusing part: `00000010` is not "ten" — it's `1×16 + 0 = 16`.

**Key point:** the file has **no lines and no rows.** It's one unbroken stream of bytes. The 16-per-row layout is a _display decision_ the program makes (like word-wrap), not a property of the file. Newlines in the source (`0x0a`) are just ordinary bytes in the stream — they do NOT cause a row break.


---

### 4. BIT MANIPULATION(THE HEART OF THE HEX CONVERSION)
### The lookup table

C has no dictionary, but you don't need one when your keys are dense integers `0..15`:
```C
char table[] = "0123456789abcdef";
```

An **array index IS the key.** `table[14]` returns `'e'`. So a nibble value (0–15) can be used _directly_ as an index — no lookup, no conversion. The number 14 physically _is_ the offset into the array.

### Extracting the two nibbles

- **Low nibble → AND with a mask:** `byte & 0x0F`
    - `&` compares bit-by-bit; result bit is 1 only if _both_ inputs are 1.
    - `0x0F` = `0000 1111` keeps the bottom 4 bits, zeroes the top 4.
    - `0110 1110 & 0000 1111 = 0000 1110` = 14. ✓

- **High nibble → right shift:** `byte >> 4`
    - `>>` slides all bits right by N; bits falling off the right are discarded.
    - Shifting right by 4 drops the low nibble and moves the high nibble to the bottom.
    - `0110 1110 >> 4 = 0000 0110` = 6. ✓
    - Masking alone can't do the high nibble: `byte & 0xF0` isolates it but leaves it in the high position, where `0110 0000` reads as **96**, not 6. **AND can only clear/keep bits — it cannot MOVE them.** Shifting is what repositions them.

### The general rule (memorize this)

> **To extract any bit-field: shift it down to the bottom, then AND-mask to its width:** `(value >> shift) & mask`

- High nibble is the special case where the mask is unneeded (shift already cleared everything above — but only because the value is a single byte AND unsigned).
- Low nibble is the special case where the shift is unneeded (already at the bottom).

### Left shift (`<<`) — for later, not used here

- Always pads new right-side slots with `0` (no signed/unsigned drama — the right end isn't the sign end).
- `<< 1` = multiply by 2. Danger is **overflow**: bits shifted off the top are lost.

---
### 5. SIGNED VS UNSIGNED - WHY `unsigned char` matters

### Reason A — for the bytes (`unsigned char buffer[]`): protect the bits

When `>>` shifts right, the empty slots on the **left** must be filled. What fills them depends on the type:

- **unsigned** → fills with **0** (logical shift). Clean 0–15 every time.
- **signed** → fills with **copies of the old top bit** (arithmetic shift / _sign extension_), because for signed numbers the top bit is the "negative" flag and `>>` is meant to preserve sign.

Bug this prevents: `0xF6 >> 4`

```
unsigned:  1111 0110 → 0000 1111 = 15  ✓  → table[15]='f'
signed:    1111 0110 → 1111 1111 = 255 ✗  → table[255] = out of bounds!
```

### Reason B — for counts/offsets (`size_t`): model a non-negative quantity

- An offset and a byte count **can never be negative** — "position −3" is meaningless. Using unsigned tells the truth about the data.
- `size_t` is **what the library speaks**: `fread` returns `size_t`, `sizeof` gives `size_t`, array indices are effectively `size_t`. Matching it means no conversions, no mismatches.
- Bigger positive range (no bit spent on sign) → can measure any object in memory.

### The unsigned footgun (respect, don't fear)

Unsigned arithmetic **wraps** instead of going negative. `for (size_t i = n; i >= 0; i--)` is an **infinite loop** — when `i` "should" hit −1 it becomes the largest `size_t`.


---

### 6. READING A FILE IN C
- `fopen(filename, "rb")` - open in **binary mode(b)**. Returns a FILE * pointer, or **NULL** on failure → always check for `NULL`.
- `fread(buffer, 1, READ_SIZE, fp)` - read up to `READ_SIZE` bytes into the `buffer`; returns **how many bytes were actually read (size_t)**. This return value is the key to everything(That's like a stretch though, i mean its not the conversion algorithm):
	- It's how i know when the file ends(return 0).
	- It's how i handle the short last row(returns < 16).
- **Streaming vs whole-file:** We chose to **stream** 16bytes at a time rather than load the whole file. This is why i dropped the earlier `fseek/ftell` dance, sorry size calculation - we never needed the total size; we only need "give me the next chunk, how many did i get?" I think i'll explain this in a separate architecture file or something

**The idiomatic read loop**
```c
while ((bytes_read = fread(buffer, 1, READ_SIZE, fp)) > 0) { ... }
```

- Reads and tests in one move. This avoids the **uninitialized variable trap**: a plain `while (bytes_read > 0)` reads `bytes_read` before its ever assigned which results in a garbage value equals **undefined behavior**
### After the loop: why did it stop?

`fread` returning 0 is ambiguous (end-of-file OR error). Disambiguate:

- `ferror(fp)` → a read error happened.
- `feof(fp)` → clean end of file.

---

### 7. THE OUTPUT LOOPS
**Hex Loop - fixed width for alignment**
Loop `i` from 0 to 16(not to **bytes_read** anymore), so every row's hex column is the same width and the ASCII gutter always starts at the same column:

```c
char table[] = "0123456789abcdef";

for (size_t i = 0; i < 16; i++)
{
	if (bytes_read < 16) //If i still counts into the actual bytes read
	{
		//Print the actual hex values
	}
	
	else // There's no more bytes to read, but i is still counting
	{
		//Print spaces
	}
	
	if (i % 2 != 0)
	{
		//print spaces
	}
}
```

- The first check is guard which is critical: As this only allows us to print actual valid bytes

### The group-separator fencepost

`xxd` groups bytes in pairs: space after bytes at index **1, 3, 5…** (odd), i.e. after the byte that _completes_ a pair. Condition: `i % 2 != 0` (odd). A first attempt used `i % 2 == 0` (even), which put the space after the _first_ byte of each pair, splitting pairs down the middle. **Lesson: derive fenceposts on paper** — write indices 0,1,2,3, mark where the thing should happen, read off the pattern. Don't twiddle randomly.


### Gutter loop — printable check

Loop `j` over the **actual** `bytes_read` (short is fine — the gutter has no fixed width to maintain). For each byte:

c

```c
if (buffer[j] >= 32 && buffer[j] <= 126)  print it as a char;
else                                      print '.';
```

- Printable ASCII range is **32 (space) to 126 (`~`)** inclusive. Below 32 = control chars; 127+ = non-printable/extended.
- **Why the check is mandatory:** printing a raw non-printable byte lets the _terminal execute it_ — `0x0a` makes a real newline mid-gutter, `0x09` jumps a tab, high bytes print garbage. The `.` substitution is what keeps the output a clean grid.



/home/kernelghost/Downloads/dev_unpacked/CHALLENGES - YEAH