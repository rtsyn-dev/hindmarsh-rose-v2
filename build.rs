fn main() {
    println!("cargo:rerun-if-changed=src/hindmarsh_rose_v2.cpp");
    println!("cargo:rerun-if-changed=src/hindmarsh_rose_v2.h");
    cc::Build::new()
        .cpp(true)
        .file("src/hindmarsh_rose_v2.cpp")
        .compile("hindmarsh_rose_v2_core");
}
