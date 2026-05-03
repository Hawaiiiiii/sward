// SwardExportFunctionContext.java
// Ghidra headless post-script for SWARD retail/runtime archaeology.
//
// Usage:
//   analyzeHeadless <projectDir> <projectName> -import <exe> \
//     -scriptPath <thisDir> -postScript SwardExportFunctionContext.java \
//     <outputJson> <targetNameOrAddress>...

import ghidra.app.script.GhidraScript;
import ghidra.program.model.address.Address;
import ghidra.program.model.listing.Function;
import ghidra.program.model.listing.FunctionManager;
import ghidra.program.model.listing.Instruction;
import ghidra.program.model.listing.InstructionIterator;
import ghidra.program.model.mem.MemoryBlock;
import ghidra.program.model.symbol.Reference;
import ghidra.program.model.symbol.ReferenceIterator;
import ghidra.program.model.symbol.ReferenceManager;
import ghidra.program.model.symbol.Symbol;
import ghidra.program.model.symbol.SymbolIterator;
import ghidra.program.model.symbol.SymbolTable;

import java.io.File;
import java.io.FileWriter;
import java.nio.charset.StandardCharsets;
import java.util.ArrayList;
import java.util.LinkedHashMap;
import java.util.List;

public class SwardExportFunctionContext extends GhidraScript {
    private static final String SCHEMA = "sward-ghidra-function-context-v1";
    private static final int MAX_REFERENCES = 64;
    private static final String[] DEFAULT_TARGETS = new String[] {
        "sub_824D6C18",
        "sub_8231C590",
        "sub_8231C5F0",
        "sub_8231C628",
        "sub_82519FE8",
        "sub_82BDBA20",
        "sub_82BDBA60",
        "sub_830BF640",
        "sub_830BF090",
        "sub_830BF300",
        "sub_830BF080"
    };

    @Override
    protected void run() throws Exception {
        String[] args = getScriptArgs();
        if (args.length < 1) {
            throw new IllegalArgumentException(
                "Expected output JSON path as first post-script argument.");
        }

        File outputFile = new File(args[0]);
        File parent = outputFile.getParentFile();
        if (parent != null) {
            parent.mkdirs();
        }

        List<String> targets = new ArrayList<>();
        for (int i = 1; i < args.length; i++) {
            if (args[i] != null && !args[i].trim().isEmpty()) {
                targets.add(args[i].trim());
            }
        }
        if (targets.isEmpty()) {
            for (String target : DEFAULT_TARGETS) {
                targets.add(target);
            }
        }

        StringBuilder json = new StringBuilder(32768);
        appendLine(json, "{");
        appendJsonField(json, "schema", SCHEMA, true, 2);
        appendProgramInfo(json);
        appendLine(json, "  \"targets\": [");
        for (int i = 0; i < targets.size(); i++) {
            monitor.checkCancelled();
            appendTarget(json, targets.get(i), i < targets.size() - 1);
        }
        appendLine(json, "  ]");
        appendLine(json, "}");

        try (FileWriter writer = new FileWriter(outputFile, StandardCharsets.UTF_8)) {
            writer.write(json.toString());
        }

        println("SWARD Ghidra function context exported: " + outputFile.getAbsolutePath());
    }

    private void appendProgramInfo(StringBuilder json) {
        appendLine(json, "  \"program\": {");
        appendJsonField(json, "name", currentProgram.getName(), true, 4);
        appendJsonField(json, "executablePath", currentProgram.getExecutablePath(), true, 4);
        appendJsonField(json, "imageBase", formatAddress(currentProgram.getImageBase()), true, 4);
        appendJsonField(json, "languageId", currentProgram.getLanguageID().toString(), true, 4);
        appendJsonField(json, "compilerSpecId", currentProgram.getCompilerSpec().getCompilerSpecID().toString(), true, 4);
        appendLine(json, "    \"memoryBlocks\": [");
        MemoryBlock[] blocks = currentProgram.getMemory().getBlocks();
        for (int i = 0; i < blocks.length; i++) {
            MemoryBlock block = blocks[i];
            appendLine(json, "      {");
            appendJsonField(json, "name", block.getName(), true, 8);
            appendJsonField(json, "start", formatAddress(block.getStart()), true, 8);
            appendJsonField(json, "end", formatAddress(block.getEnd()), true, 8);
            appendJsonField(json, "size", Long.toString(block.getSize()), false, 8);
            appendLine(json, "      }" + (i < blocks.length - 1 ? "," : ""));
        }
        appendLine(json, "    ]");
        appendLine(json, "  },");
    }

    private void appendTarget(StringBuilder json, String query, boolean trailingComma) {
        Function function = resolveFunction(query);
        appendLine(json, "    {");
        appendJsonField(json, "query", query, true, 6);

        if (function == null) {
            appendJsonField(json, "status", "unresolved", true, 6);
            appendJsonField(json, "name", "", true, 6);
            appendJsonField(json, "entry", "", true, 6);
            appendLine(json, "      \"callers\": [],");
            appendLine(json, "      \"callees\": [],");
            appendLine(json, "      \"referencesTo\": [],");
            appendLine(json, "      \"referencesFrom\": []");
            appendLine(json, "    }" + (trailingComma ? "," : ""));
            return;
        }

        appendJsonField(json, "status", "resolved", true, 6);
        appendJsonField(json, "name", function.getName(), true, 6);
        appendJsonField(json, "entry", formatAddress(function.getEntryPoint()), true, 6);
        appendJsonField(json, "rva", formatRva(function.getEntryPoint()), true, 6);
        appendJsonField(json, "bodyMin", formatAddress(function.getBody().getMinAddress()), true, 6);
        appendJsonField(json, "bodyMax", formatAddress(function.getBody().getMaxAddress()), true, 6);
        appendFunctionList(json, "callers", collectCallers(function), true, 6);
        appendFunctionList(json, "callees", collectCallees(function), true, 6);
        appendReferenceList(json, "referencesTo", collectReferencesTo(function), true, 6);
        appendReferenceList(json, "referencesFrom", collectReferencesFrom(function), false, 6);
        appendLine(json, "    }" + (trailingComma ? "," : ""));
    }

    private Function resolveFunction(String query) {
        Function bySymbol = resolveFunctionBySymbol(query);
        if (bySymbol != null) {
            return bySymbol;
        }

        if (!query.startsWith("__imp__")) {
            bySymbol = resolveFunctionBySymbol("__imp__" + query);
            if (bySymbol != null) {
                return bySymbol;
            }
        }

        String hex = query;
        if (hex.startsWith("sub_")) {
            hex = hex.substring(4);
        }
        if (hex.startsWith("0x") || hex.startsWith("0X")) {
            hex = hex.substring(2);
        }
        if (!hex.matches("[0-9A-Fa-f]+")) {
            return null;
        }

        try {
            Address address = toAddr("0x" + hex);
            return resolveFunctionAtOrContaining(address);
        }
        catch (Exception ignored) {
            return null;
        }
    }

    private Function resolveFunctionBySymbol(String name) {
        SymbolTable symbolTable = currentProgram.getSymbolTable();
        for (Symbol symbol : symbolTable.getGlobalSymbols(name)) {
            Function function = resolveFunctionAtOrContaining(symbol.getAddress());
            if (function != null) {
                return function;
            }
        }

        SymbolIterator symbols = symbolTable.getAllSymbols(true);
        while (symbols.hasNext()) {
            Symbol symbol = symbols.next();
            if (!name.equals(symbol.getName())) {
                continue;
            }
            Function function = resolveFunctionAtOrContaining(symbol.getAddress());
            if (function != null) {
                return function;
            }
        }
        return null;
    }

    private Function resolveFunctionAtOrContaining(Address address) {
        FunctionManager functionManager = currentProgram.getFunctionManager();
        Function function = functionManager.getFunctionAt(address);
        if (function != null) {
            return function;
        }
        return functionManager.getFunctionContaining(address);
    }

    private List<Function> collectCallers(Function function) {
        LinkedHashMap<String, Function> callers = new LinkedHashMap<>();
        ReferenceManager referenceManager = currentProgram.getReferenceManager();
        ReferenceIterator references = referenceManager.getReferencesTo(function.getEntryPoint());
        while (references.hasNext() && callers.size() < MAX_REFERENCES) {
            Reference reference = references.next();
            Function caller = currentProgram.getFunctionManager().getFunctionContaining(reference.getFromAddress());
            if (caller != null) {
                callers.put(formatAddress(caller.getEntryPoint()), caller);
            }
        }
        return new ArrayList<>(callers.values());
    }

    private List<Function> collectCallees(Function function) {
        LinkedHashMap<String, Function> callees = new LinkedHashMap<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
        while (instructions.hasNext() && callees.size() < MAX_REFERENCES) {
            Instruction instruction = instructions.next();
            for (Reference reference : instruction.getReferencesFrom()) {
                Function callee = currentProgram.getFunctionManager().getFunctionAt(reference.getToAddress());
                if (callee == null) {
                    callee = currentProgram.getFunctionManager().getFunctionContaining(reference.getToAddress());
                }
                if (callee != null && !callee.getEntryPoint().equals(function.getEntryPoint())) {
                    callees.put(formatAddress(callee.getEntryPoint()), callee);
                }
            }
        }
        return new ArrayList<>(callees.values());
    }

    private List<Reference> collectReferencesTo(Function function) {
        List<Reference> result = new ArrayList<>();
        ReferenceIterator references = currentProgram.getReferenceManager().getReferencesTo(function.getEntryPoint());
        while (references.hasNext() && result.size() < MAX_REFERENCES) {
            result.add(references.next());
        }
        return result;
    }

    private List<Reference> collectReferencesFrom(Function function) {
        List<Reference> result = new ArrayList<>();
        InstructionIterator instructions = currentProgram.getListing().getInstructions(function.getBody(), true);
        while (instructions.hasNext() && result.size() < MAX_REFERENCES) {
            Instruction instruction = instructions.next();
            for (Reference reference : instruction.getReferencesFrom()) {
                if (result.size() >= MAX_REFERENCES) {
                    break;
                }
                result.add(reference);
            }
        }
        return result;
    }

    private void appendFunctionList(
            StringBuilder json,
            String name,
            List<Function> functions,
            boolean trailingComma,
            int indent) {
        appendLine(json, spaces(indent) + "\"" + name + "\": [");
        for (int i = 0; i < functions.size(); i++) {
            Function function = functions.get(i);
            appendLine(json, spaces(indent + 2) + "{");
            appendJsonField(json, "name", function.getName(), true, indent + 4);
            appendJsonField(json, "entry", formatAddress(function.getEntryPoint()), true, indent + 4);
            appendJsonField(json, "rva", formatRva(function.getEntryPoint()), false, indent + 4);
            appendLine(json, spaces(indent + 2) + "}" + (i < functions.size() - 1 ? "," : ""));
        }
        appendLine(json, spaces(indent) + "]" + (trailingComma ? "," : ""));
    }

    private void appendReferenceList(
            StringBuilder json,
            String name,
            List<Reference> references,
            boolean trailingComma,
            int indent) {
        appendLine(json, spaces(indent) + "\"" + name + "\": [");
        for (int i = 0; i < references.size(); i++) {
            Reference reference = references.get(i);
            appendLine(json, spaces(indent + 2) + "{");
            appendJsonField(json, "from", formatAddress(reference.getFromAddress()), true, indent + 4);
            appendJsonField(json, "to", formatAddress(reference.getToAddress()), true, indent + 4);
            appendJsonField(json, "type", reference.getReferenceType().toString(), false, indent + 4);
            appendLine(json, spaces(indent + 2) + "}" + (i < references.size() - 1 ? "," : ""));
        }
        appendLine(json, spaces(indent) + "]" + (trailingComma ? "," : ""));
    }

    private void appendJsonField(StringBuilder json, String name, String value, boolean trailingComma, int indent) {
        appendLine(
            json,
            spaces(indent) + "\"" + escapeJson(name) + "\": \"" + escapeJson(value) + "\"" + (trailingComma ? "," : ""));
    }

    private String formatAddress(Address address) {
        if (address == null) {
            return "";
        }
        return "0x" + address.toString(false).toUpperCase();
    }

    private String formatRva(Address address) {
        try {
            long rva = address.subtract(currentProgram.getImageBase());
            if (rva < 0) {
                return "";
            }
            return "0x" + Long.toHexString(rva).toUpperCase();
        }
        catch (Exception ignored) {
            return "";
        }
    }

    private static void appendLine(StringBuilder json, String line) {
        json.append(line).append('\n');
    }

    private static String spaces(int count) {
        StringBuilder result = new StringBuilder(count);
        for (int i = 0; i < count; i++) {
            result.append(' ');
        }
        return result.toString();
    }

    private static String escapeJson(String value) {
        if (value == null) {
            return "";
        }
        StringBuilder escaped = new StringBuilder(value.length() + 16);
        for (int i = 0; i < value.length(); i++) {
            char ch = value.charAt(i);
            switch (ch) {
                case '\\':
                    escaped.append("\\\\");
                    break;
                case '"':
                    escaped.append("\\\"");
                    break;
                case '\n':
                    escaped.append("\\n");
                    break;
                case '\r':
                    escaped.append("\\r");
                    break;
                case '\t':
                    escaped.append("\\t");
                    break;
                default:
                    escaped.append(ch);
                    break;
            }
        }
        return escaped.toString();
    }
}
