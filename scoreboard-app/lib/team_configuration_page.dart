import 'dart:async';
import 'package:flutter/material.dart';
import 'web_socket_service.dart';
import 'team_model.dart';

class TeamConfigurationPage extends StatefulWidget {
  final WebSocketService wsService;

  const TeamConfigurationPage({super.key, required this.wsService});

  @override
  State<TeamConfigurationPage> createState() => _TeamConfigurationPageState();
}

class _TeamConfigurationPageState extends State<TeamConfigurationPage> {
  List<Team> _teams = [];
  bool _isLoading = true;
  StreamSubscription? _subscription;

  @override
  void initState() {
    super.initState();
    _subscription = widget.wsService.teamsStream.listen((teams) {
      if (mounted) {
        setState(() {
          _teams = teams;
          _isLoading = false;
        });
      }
    });
    widget.wsService.getTeams();
  }

  @override
  void dispose() {
    _subscription?.cancel();
    super.dispose();
  }

  void _showAddTeamDialog() {
    final controller = TextEditingController();
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Add New Team'),
        content: TextField(
          controller: controller,
          decoration: const InputDecoration(labelText: 'Team Name'),
          autofocus: true,
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              if (controller.text.isNotEmpty) {
                widget.wsService.addOrUpdatePlayer(controller.text, "New Team", 0);
                Navigator.pop(context);
              }
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  void _showAddPlayerDialog(Team team) {
    final nameController = TextEditingController();
    final numberController = TextEditingController();
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Add Player to ${team.name}'),
        content: Column(
          mainAxisSize: MainAxisSize.min,
          children: [
            TextField(
              controller: nameController,
              decoration: const InputDecoration(labelText: 'Player Name'),
              autofocus: true,
            ),
            TextField(
              controller: numberController,
              decoration: const InputDecoration(labelText: 'Number'),
              keyboardType: TextInputType.number,
            ),
          ],
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              int? num = int.tryParse(numberController.text);
              if (nameController.text.isNotEmpty && num != null) {
                widget.wsService.addOrUpdatePlayer(team.name, nameController.text, num);
                Navigator.pop(context);
              }
            },
            child: const Text('Add'),
          ),
        ],
      ),
    );
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text('Team Configuration'),
        actions: [
          IconButton(
            icon: const Icon(Icons.refresh),
            onPressed: () => widget.wsService.getTeams(),
          ),
        ],
      ),
      body: _isLoading 
        ? const Center(child: CircularProgressIndicator())
        : ListView.builder(
            itemCount: _teams.length,
            itemBuilder: (context, index) {
              final team = _teams[index];
              return Card(
                margin: const EdgeInsets.all(8.0),
                child: ExpansionTile(
                  title: Text(team.name, style: const TextStyle(fontWeight: FontWeight.bold, fontSize: 18)),
                  trailing: IconButton(
                    icon: const Icon(Icons.delete, color: Colors.red),
                    onPressed: () => _confirmDeleteTeam(team),
                  ),
                  children: [
                    ListView.builder(
                      shrinkWrap: true,
                      physics: const NeverScrollableScrollPhysics(),
                      itemCount: team.players.length,
                      itemBuilder: (context, pIndex) {
                        final player = team.players[pIndex];
                        if (player.number == 0 && player.name == "New Team") return const SizedBox.shrink();
                        return ListTile(
                          leading: CircleAvatar(child: Text('${player.number}')),
                          title: Text(player.name),
                          trailing: IconButton(
                            icon: const Icon(Icons.remove_circle_outline),
                            onPressed: () => widget.wsService.removePlayer(team.name, player.number),
                          ),
                          onTap: () => _showEditPlayerDialog(team, player),
                        );
                      },
                    ),
                    Padding(
                      padding: const EdgeInsets.all(8.0),
                      child: ElevatedButton.icon(
                        onPressed: () => _showAddPlayerDialog(team),
                        icon: const Icon(Icons.add),
                        label: const Text('Add Player'),
                      ),
                    ),
                  ],
                ),
              );
            },
          ),
      floatingActionButton: FloatingActionButton(
        onPressed: _showAddTeamDialog,
        child: const Icon(Icons.group_add),
      ),
    );
  }

  void _showEditPlayerDialog(Team team, Player player) {
    final nameController = TextEditingController(text: player.name);
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: Text('Edit Player #${player.number}'),
        content: TextField(
          controller: nameController,
          decoration: const InputDecoration(labelText: 'Player Name'),
          autofocus: true,
        ),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              if (nameController.text.isNotEmpty) {
                widget.wsService.addOrUpdatePlayer(team.name, nameController.text, player.number);
                Navigator.pop(context);
              }
            },
            child: const Text('Save'),
          ),
        ],
      ),
    );
  }

  void _confirmDeleteTeam(Team team) {
    showDialog(
      context: context,
      builder: (context) => AlertDialog(
        title: const Text('Delete Team'),
        content: Text('Are you sure you want to delete the team "${team.name}"?'),
        actions: [
          TextButton(onPressed: () => Navigator.pop(context), child: const Text('Cancel')),
          ElevatedButton(
            onPressed: () {
              widget.wsService.deleteTeam(team.name);
              Navigator.pop(context);
            },
            style: ElevatedButton.styleFrom(backgroundColor: Colors.red, foregroundColor: Colors.white),
            child: const Text('Delete'),
          ),
        ],
      ),
    );
  }
}
