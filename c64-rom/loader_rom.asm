.file [name="loader_rom.bin", type="bin", segments="Code,CopySource,CommandArea"]

.label nufli_dest = $2000   // NUFLI destination start
.label nufli_exec = $3000   // NUFLI displayer entry point
.const NUFLI_SIZE = $5a00   // unpacked NUFLI size (size of raspi.bin)
.const NUM_1K_NUFLI_BLOX = floor(NUFLI_SIZE/1024)  // number of full 1K NUFLI blocks
                                                   // (remainder is not counted)

//
// Zero-page pointers (used to copy to different offsets into nufli_dest)
//
.label dest_ptr1 = $f8
.label dest_ptr2 = $fa
.label dest_ptr3 = $fc
.label dest_ptr4 = $fe

//
// 1k window to copy NUFLI from
//
.segmentdef CopySource [min=$8400, max=$87ff]
.label copy_source = $8400


//
// 256 byte window to send commands by reading from
//
.segmentdef CommandArea [min=$9e00, max=$9eff]
.label command_area = $9e00
.const CMD_GET_STATUS = 0
.const CMD_NEXT_PAGE = 1


.segment Code [start=$8000]

*=$8000                         // cartridge header
.word start                     // start address
.word start                     // NMI ISR address
.encoding "petscii_mixed"       
.text "CBM80"                   // cartridge signature
.encoding "screencode_upper"

start:  jsr $ff81               // kernal: initialize VIC-II
        jsr $ff84               // kernal: initialize CIA

        // show a black screen while we load the NUFLI data
        lda #0
        sta $d020               // black border
        sta $d021               // black background
        lda #' '                // clear with spaces
        ldx #0
clear:  sta $0400, x
        sta $0500, x
        sta $0600, x
        sta $0700, x
        dex
        bne clear

        ldx #NUM_1K_NUFLI_BLOX  // copy the NUFLI 1 KB at a time
        lda #<nufli_dest        // put copy destinations in $f8,$fa,$fc,$fe
        sta dest_ptr1
        sta dest_ptr2
        sta dest_ptr3
        sta dest_ptr4
        lda #>nufli_dest
        sta dest_ptr1 + 1
        lda #>(nufli_dest + $100)
        sta dest_ptr2 + 1
        lda #>(nufli_dest + $200)
        sta dest_ptr3 + 1
        lda #>(nufli_dest + $300)
        sta dest_ptr4 + 1

        ldy #0
copy1k: lda command_area + CMD_GET_STATUS
        sta $d020               // flash border if the pico's cpu is busy
        bne copy1k              // loop until status is not busy
        lda copy_source, y
        sta (dest_ptr1), y
        lda copy_source + $100, y
        sta (dest_ptr2), y
        lda copy_source + $200, y
        sta (dest_ptr3), y
        lda copy_source + $300, y
        sta (dest_ptr4), y
        iny
        bne copy1k

        lda command_area + CMD_NEXT_PAGE    // advance the source window by 1 KB
        clc
        lda #4                              // advance the dest addrs by 1 KB (high byte of #$400)
        adc dest_ptr1 + 1
        sta dest_ptr1 + 1
        lda #4
        adc dest_ptr2 + 1
        sta dest_ptr2 + 1
        lda #4
        adc dest_ptr3 + 1
        sta dest_ptr3 + 1
        lda #4
        adc dest_ptr4 + 1
        sta dest_ptr4 + 1

        dex                     // loop copying whole KBs
        bne copy1k

        // copy the remaining data 256 bytes at a time
        // (could be optimized by trying 768 at a time, then 512, then up to 256)
        ldx #0
.const BYTES_LEFT = NUFLI_SIZE & $03ff
.for(var i = 0; i < BYTES_LEFT; i += 256) {
        .if (BYTES_LEFT - i < 256) {
                ldx #(BYTES_LEFT - i)
        }
copy:   lda copy_source + i, x
        sta nufli_dest + NUM_1K_NUFLI_BLOX*1024 + i, x
        inx
        bne copy
}
        jmp nufli_exec          // Finished copying! Execute!
