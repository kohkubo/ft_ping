# パケットロスを発生させる設定

```bash
tc qdisc add dev eth0 root netem loss 50%
```

# パケット破損を発生させる設定

```bash
tc qdisc add dev eth0 root netem corrupt 50%
```

# パケット遅延を発生させる設定

```bash
tc qdisc add dev eth0 root netem delay 100ms 10ms
```

# パケット重複を発生させる設定

```bash
tc qdisc add dev eth0 root netem duplicate 10%
```



# 設定を確認

```bash
tc qdisc show dev eth0
```

# 設定を解除

```bash
tc qdisc del dev eth0 root
```
