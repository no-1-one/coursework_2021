podman run -d -v "$(pwd)"/envoy.yaml:/etc/envoy/envoy.yaml:ro  --network=host envoyproxy/envoy:v1.15.0

