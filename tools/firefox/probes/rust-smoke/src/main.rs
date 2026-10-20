//! Exercise the OS facilities Firefox's Rust dependencies need on Haiku/RISC-V.
use std::cell::Cell;
use std::io::{Read, Write};
use std::net::{TcpListener, TcpStream};
use std::sync::atomic::{AtomicU64, Ordering};
use std::sync::{Arc, Barrier};
use std::time::Duration;

thread_local! { static LOCAL: Cell<u64> = const { Cell::new(0) }; }

fn main() -> std::io::Result<()> {
    if std::env::args().nth(1).as_deref() == Some("--child") {
        assert_eq!(std::env::var("FIREFOX_PROBE_CHILD").as_deref(), Ok("1"));
        println!("child-ok");
        return Ok(());
    }
    assert_eq!(std::env::consts::OS, "haiku");
    assert_eq!(std::env::consts::ARCH, "riscv64");
    let count = Arc::new(AtomicU64::new(0));
    let barrier = Arc::new(Barrier::new(5));
    let mut workers = Vec::new();
    for id in 1..=4 {
        let count = Arc::clone(&count);
        let barrier = Arc::clone(&barrier);
        workers.push(std::thread::spawn(move || {
            LOCAL.with(|slot| { assert_eq!(slot.get(), 0); slot.set(id); });
            barrier.wait();
            for _ in 0..10_000 { count.fetch_add(1, Ordering::SeqCst); }
            LOCAL.with(|slot| assert_eq!(slot.get(), id));
        }));
    }
    barrier.wait();
    for worker in workers { worker.join().expect("worker failed"); }
    assert_eq!(count.load(Ordering::SeqCst), 40_000);
    LOCAL.with(|slot| assert_eq!(slot.get(), 0));
    println!("threads/barrier/TLS/64-bit atomics: PASS");

    let path = std::env::temp_dir().join(format!("firefox-rust-smoke-{}", std::process::id()));
    let mut file = std::fs::OpenOptions::new().write(true).create_new(true).open(&path)?;
    file.write_all(b"Haiku RISC-V\n")?;
    file.sync_all()?;
    drop(file);
    assert_eq!(std::fs::read(&path)?, b"Haiku RISC-V\n");
    std::fs::remove_file(&path)?;
    println!("file creation/write/sync/read: PASS");

    let child = std::process::Command::new(std::env::current_exe()?)
        .arg("--child").env("FIREFOX_PROBE_CHILD", "1").output()?;
    assert!(child.status.success(), "child process failed");
    assert_eq!(child.stdout, b"child-ok\n");
    println!("child process/arguments/environment/stdout pipe: PASS");

    let listener = TcpListener::bind(("127.0.0.1", 0))?;
    let address = listener.local_addr()?;
    let server = std::thread::spawn(move || -> std::io::Result<()> {
        let (mut stream, _) = listener.accept()?;
        stream.set_read_timeout(Some(Duration::from_secs(5)))?;
        let mut request = [0; 4];
        stream.read_exact(&mut request)?;
        assert_eq!(&request, b"ping");
        stream.write_all(b"pong")
    });
    let mut client = TcpStream::connect_timeout(&address, Duration::from_secs(5))?;
    client.set_read_timeout(Some(Duration::from_secs(5)))?;
    client.write_all(b"ping")?;
    let mut response = [0; 4];
    client.read_exact(&mut response)?;
    assert_eq!(&response, b"pong");
    server.join().expect("server failed")?;
    println!("loopback TCP/read/write/timeouts: PASS");
    println!("RUST_SMOKE_PASS (not a Firefox browser test)");
    Ok(())
}
