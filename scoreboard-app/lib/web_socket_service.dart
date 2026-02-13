import 'dart:convert';
import 'dart:async';
import 'package:web_socket_channel/web_socket_channel.dart';
import 'scoreboard_state.dart';

enum ConnectionStatus { connected, disconnected, connecting }

class WebSocketService {
  WebSocketChannel? _channel;
  final _stateController = StreamController<ScoreboardState>.broadcast();
  final _connectionController = StreamController<ConnectionStatus>.broadcast();
  
  Stream<ScoreboardState> get stateStream => _stateController.stream;
  Stream<ConnectionStatus> get connectionStream => _connectionController.stream;
  
  ConnectionStatus _status = ConnectionStatus.disconnected;
  ConnectionStatus get status => _status;

  String? _lastHost;
  int? _lastPort;
  bool _shouldReconnect = false;
  Timer? _reconnectTimer;

  Future<void> connect(String host, int port) async {
    _lastHost = host;
    _lastPort = port;
    _shouldReconnect = true;
    _doConnect();
  }

  void _updateStatus(ConnectionStatus status) {
    _status = status;
    _connectionController.add(status);
  }

  Future<void> _doConnect() async {
    if (_lastHost == null || _lastPort == null) return;
    
    _reconnectTimer?.cancel();
    _updateStatus(ConnectionStatus.connecting);
    
    final uri = Uri.parse('ws://$_lastHost:$_lastPort');
    
    try {
      _channel = WebSocketChannel.connect(uri);
      
      // Wait for the first message or an error to confirm connection
      // WebSocketChannel.connect is lazy, it doesn't actually connect until someone listens
      _channel!.stream.listen((message) {
        if (_status != ConnectionStatus.connected) {
          _updateStatus(ConnectionStatus.connected);
        }
        
        try {
          final Map<String, dynamic> data = jsonDecode(message);
          final state = ScoreboardState.fromJson(data);
          _stateController.add(state);
        } catch (e) {
          print('Error parsing scoreboard state: $e');
        }
      }, onDone: () {
        _updateStatus(ConnectionStatus.disconnected);
        _scheduleReconnect();
      }, onError: (error) {
        _updateStatus(ConnectionStatus.disconnected);
        _scheduleReconnect();
      });
    } catch (e) {
      _updateStatus(ConnectionStatus.disconnected);
      _scheduleReconnect();
    }
  }

  void _scheduleReconnect() {
    if (!_shouldReconnect) return;
    
    _reconnectTimer?.cancel();
    _reconnectTimer = Timer(const Duration(seconds: 3), () {
      if (_status == ConnectionStatus.disconnected) {
        _doConnect();
      }
    });
  }

  void sendCommand(String command, {dynamic value, int? delta, int? index, int? player, int? minutes, int? seconds}) {
    if (_channel == null || _status != ConnectionStatus.connected) return;
    
    final Map<String, dynamic> payload = {'command': command};
    if (value != null) payload['value'] = value;
    if (delta != null) payload['delta'] = delta;
    if (index != null) payload['index'] = index;
    if (player != null) payload['player'] = player;
    if (minutes != null) payload['minutes'] = minutes;
    if (seconds != null) payload['seconds'] = seconds;

    try {
      _channel!.sink.add(jsonEncode(payload));
    } catch (e) {
      print('Failed to send command: $e');
    }
  }

  void dispose() {
    _shouldReconnect = false;
    _reconnectTimer?.cancel();
    _channel?.sink.close();
    _stateController.close();
    _connectionController.close();
  }
}
