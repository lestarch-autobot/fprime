# Data Transport Plugin

Data Transport Plugins swap out the link between the GDS data pipeline (`StandardPipeline`) and the ground system that holds the deployment's link. By default the pipeline connects to the GDS communication process over ZeroMQ. A data transport plugin lets the same pipeline, and therefore the GDS GUI, `fprime-cli`, and the Integration Test API, obtain commands, events, and telemetry through another ground system (for example YAMCS or COSMOS) without changes to the consumers.

Data transport is a `SELECTION` type plugin meaning exactly one data transport runs in a pipeline. It is selected with `--data-transport-selection <name>`.

The F Prime GDS ships two built-in data transports:

| Name  | Class                                                      | Arguments                             | Notes                                       |
|-------|------------------------------------------------------------|---------------------------------------|---------------------------------------------|
| `zmq` | `fprime_gds.common.zmq_transport.ZmqClient`                | `--zmq-transport serverInUrl serverOutUrl` | Default. Connects to the ZeroMQ comm process |
| `tcp` | `fprime_gds.common.transport.ThreadedTCPSocketClient`      | `--tts-addr ADDR --tts-port PORT`     | Connects to the threaded TCP server (`fprime-gds` launches it for this selection) |

`--no-zmq` remains available for one release as a deprecated alias of `--data-transport-selection tcp`.

A third transport, `yamcs` (`--yamcs-url URL`), is built in while the `fprime-gds[yamcs]` extra is installed and will move to the `fprime-yamcs` package. The pytest option `--use-yamcs` remains for one release as a deprecated alias of `--data-transport-selection yamcs`.

## Usage

The selected plugin class is installed as `StandardPipeline.transport_implementation` and is constructed with the values of its own command line arguments. Every GDS entry point that builds a pipeline accepts the selection:

```bash
fprime-gds --dictionary <dictionary> --data-transport-selection tcp --tts-port 50050
fprime-cli channels --dictionary <dictionary> --data-transport-selection zmq
pytest test/int --dictionary <dictionary> --data-transport-selection <name> <plugin arguments>
```

The selection may also be set in a GDS configuration file like any other pipeline argument.

## Considerations

- `get_connection_uri(client, **arguments)` is called with `client=True` for pipeline clients (GUI, `fprime-cli`, the test API) and `client=False` for the process that owns the transport (`fprime-gds`); `client` is therefore a reserved name that transport arguments must not use. The returned URI is passed to `connect()`.
- A transport that decodes ground-system data itself (rather than forwarding raw F Prime frames) may define `set_pipeline_references(dictionaries, channel_decoder, event_decoder)`; `StandardPipeline.setup()` then hands it the dictionaries and decoders and registers it as the command consumer.

- The transport runs inside the pipeline process (GUI, CLI, or test). The pipeline hands outgoing bytes to `data_callback()`, which calls `send()`. Incoming data is delivered to the registered consumers with `send_to_all()` (push), or returned from `recv()` when subclassing `ThreadedTransportClient`, whose receive thread forwards it.
- The transport speaks the ground system's protocol, not the deployment's: the plugin receives fully encoded F Prime command bytes and must emit `ChData`/`EventData` objects (or raw F Prime records for the pipeline's decoders), as the built-in clients do.
- `fprime-gds` launches the threaded TCP server only for `tcp`, and the communication process only for `zmq` and `tcp`. With any other transport it skips the communication process (with a warning unless `--communication-selection none`) and launches the deployment binary only when explicit application arguments are supplied; the ground system is expected to hold the link to the deployment already.

## Required Interface

To create a data transport plugin, subclass [`TransportClient`](https://github.com/nasa/fprime-gds/blob/devel/src/fprime_gds/common/transport.py) and implement `connect()`, `disconnect()`, `send()`, and `recv()`. Provide `get_name()` (the selection name), and `get_arguments()`, `check_arguments()`, and `get_connection_uri()` for the plugin's command line arguments.

```python
from fprime_gds.common.transport import TransportClient
from fprime_gds.plugin.definitions import gds_plugin


@gds_plugin(TransportClient)
class ExampleGroundSystemClient(TransportClient):
    """Transport reaching an external ground system over HTTP"""

    def __init__(self, ground_url="http://localhost:8080"):
        super().__init__()
        self.ground_url = ground_url

    @classmethod
    def get_name(cls):
        return "example-ground"

    @classmethod
    def get_arguments(cls):
        return {
            ("--ground-url",): {
                "dest": "ground_url",
                "type": str,
                "default": "http://localhost:8080",
                "help": "URL of the ground system [default: %(default)s]",
            }
        }

    @classmethod
    def check_arguments(cls, ground_url):
        if not ground_url.startswith(("http://", "https://")):
            raise ValueError(f"--ground-url must be an http(s) URL, got: {ground_url}")

    @classmethod
    def get_connection_uri(cls, client=True, ground_url="http://localhost:8080"):
        return ground_url

    def connect(self, connection_uri=None, incoming_routing=None, outgoing_routing=None):
        """Open the connection to the ground system and start delivering incoming data with send_to_all()"""

    def disconnect(self):
        """Close the connection"""

    def send(self, data: bytes):
        """Forward encoded F Prime command bytes to the ground system"""

    def recv(self, timeout=None):
        """Unused for push-model transports"""
        return b""
```

Arguments follow the same `get_arguments()` / `check_arguments()` convention as other GDS plugins: the keys of `get_arguments()` are passed to `argparse`, and the parsed values are passed by `dest` name to `check_arguments()`, `get_connection_uri()`, and the constructor.

## Registration

Register the plugin class through the `fprime_gds` entry point of the package that contains it (the entry point must name the class, whose `register_data_transport_plugin` hook the plugin system discovers), as for every other GDS plugin:

```toml
[project.entry-points.fprime_gds]
example_ground = "example_package.transport:ExampleGroundSystemClient"
```

Once installed, `--data-transport-selection example-ground` selects it and its arguments appear in `--help`.
