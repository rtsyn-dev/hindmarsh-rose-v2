fn main() {
    println!("cargo:rerun-if-changed=src/hindmarsh_rose_v2.c");
    println!("cargo:rerun-if-changed=src/hindmarsh_rose_v2.h");
    cc::Build::new()
        .file("src/hindmarsh_rose_v2.c")
        .compile("hindmarsh_rose_v2_core");
}
